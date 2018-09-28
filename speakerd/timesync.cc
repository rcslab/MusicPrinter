/*
 * Simplified Time Synchronization Algorithm
 *
 * Example:
 *  Two machines where M1 is 3 seconds behind M2 and there is a one second 
 *  latency between the machines.
 *
 *  M1 2s -> M5 (recieved at 6s)
 *  Difference1: 4s = Network Latency + Clock Diff
 *
 *  M5 6s -> M1 (recieved at 4s)
 *  Difference2: -2s = Network Latency - Clock Diff
 *
 *  Network Latency is approximately | D1 + D2 | / 2
 *  Time Difference is approximately | D1 - D2 | / 2
 *
 * In practice the network has a lot jitter in network latency meaning we won't 
 * get a precise measurement of latency each time.  There's a lot fancier 
 * solutions but a simple one is for us to take many measurements and take the 
 * smallest network latency of all.
 *
 */

#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include "timesync.h"

using namespace std;

/*
 * XXX: Set to the local broadcast address of the network.  Run ifconfig to see 
 * the broadcast address for the local area network where all the machines run 
 * on.  You can also broadcast to 255.255.255.255 but machines with multiple 
 * NICs may not route the packet to the correct network.
 */
static char bcIP[] = "192.168.1.255";

#define TIMESYNC_PORT 8086

/*
 * machineTime -- Get local time in microseconds.
 */
static int64_t
machineTime()
{
    struct timeval tp;
    struct timezone tzp;

    gettimeofday(&tp, &tzp);

    return tp.tv_sec * 1000000 + tp.tv_usec;
}

TSMachine::TSMachine(uint32_t ip) : ip(ip), ts()
{
}

TSMachine::~TSMachine()
{
}

void
TSMachine::dump()
{
    char ipStr[INET_ADDRSTRLEN];

    inet_ntop(AF_INET, &ip, ipStr, INET_ADDRSTRLEN);

    printf("Machine %s\n", ipStr);
    printf("    TD %lld RemoteTD %lld\n", getTSDelta(), tdpeer);
}

void
TSMachine::addSample(int64_t localts, int64_t remotets)
{
    ts.push_front(localts - remotets);
    if (ts.size() > 120) {
        ts.pop_back();
    }
}

uint32_t
TSMachine::getIP()
{
    return ip;
}

int64_t
TSMachine::getTSDelta()
{
    int64_t min = INT64_MAX;
    for (auto &&s : ts) {
        if (min > s) {
            min = s;
        }
    }
    return min;
}

TimeSync::TimeSync()
    : done(false), myIP(0xffffffff), thrAnnounce(nullptr), thrSync(nullptr), machines()
{
}

TimeSync::~TimeSync()
{
    if (!done)
        stop();
}

void
TimeSync::start()
{
    done = false;
    thrAnnounce = new thread(&TimeSync::announcer, this);
    thrSync = new thread(&TimeSync::listener, this);
}

void
TimeSync::stop()
{
    done = true;

    thrAnnounce->join();
    delete thrAnnounce;
    thrAnnounce = nullptr;

    thrSync->join();
    delete thrSync;
    thrSync = nullptr;
}

int64_t
TimeSync::getTime()
{
    return 0;
}

void
TimeSync::sleepUntil(int64_t ts)
{
}

void
TimeSync::dump()
{
    for (auto &&m : machines) {
        m.second.dump();
    }
}

void
TimeSync::announcer()
{
    int fd;
    int status;
    int broadcast = 1;
    struct sockaddr_in dstAddr;

    fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (fd < 0) {
        perror("socket");
        abort();
    }

    status = setsockopt(fd, SOL_SOCKET, SO_BROADCAST,
                          &broadcast, sizeof(broadcast));
    if (status < 0) {
        perror("setsockopt");
        abort();
    }

    memset(&dstAddr, 0, sizeof(dstAddr));
    dstAddr.sin_family = AF_INET;
    inet_pton(AF_INET, bcIP, &dstAddr.sin_addr.s_addr);
    dstAddr.sin_port = htons(TIMESYNC_PORT);

    while (!done) {
        int i = 0;
        TSPkt pkt;

        pkt.magic = TIMESYNC_MAGIC;
        pkt.ts = machineTime();
        for (auto &&m : machines) {
            pkt.machines[i].ip = m.first;
            pkt.machines[i].td = m.second.getTSDelta();
            i++;
        }

        status = (int)sendto(fd, (char *)&pkt, sizeof(pkt), 0,
                        (struct sockaddr *)&dstAddr, sizeof(dstAddr));
        if (status < 0) {
            perror("sendto");
        }

        printf("Announcement Sent\n");
        dump();

        sleep(1);
    }
}

void
TimeSync::processPkt(uint32_t src, const TSPkt &pkt)
{
    int64_t ts = machineTime();

    if (pkt.magic != TIMESYNC_MAGIC) {
        printf("Received a corrupted timesync packet!");
        return;
    }

    if (machines.find(src) == machines.end()) {
        machines[src] = TSMachine(src);
    }

    machines[src].addSample(ts, pkt.ts);

    for (int i = 0; i < TIMESYNC_MACHINES; i++) {
        if (pkt.machines[i].ip == myIP) {
            machines[src].tdpeer = pkt.machines[i].td;
        }
    }
}

void
TimeSync::listener()
{
    int fd;
    int status;
    struct sockaddr_in addr;
    int reuseaddr = 1;
    int broadcast = 1;

    // Create a network socket
    fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (fd < 0) {
        perror("socket");
        abort();
    }

    /*
     * Ensure that we can reopen the address and port without the default 
     * timeout (usually 120 seconds) that blocks reusing addresses and ports 
     * immediately.
     */
    status = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
                        &reuseaddr, sizeof(reuseaddr));
    if (status < 0) {
        perror("setsockopt");
        abort();
    }

    status = setsockopt(fd, SOL_SOCKET, SO_REUSEPORT,
                        &reuseaddr, sizeof(reuseaddr));
    if (status < 0) {
        perror("setsockopt");
        abort();
    }

    // Make this a broadcast socket
    status = setsockopt(fd, SOL_SOCKET, SO_BROADCAST,
                          &broadcast, sizeof(broadcast));
    if (status < 0) {
        perror("setsockopt");
        abort();
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(TIMESYNC_PORT);

    // Bind to the address/port we want to listen to
    status = ::bind(fd, (struct sockaddr *)&addr, sizeof(addr));
    if (status < 0) {
        perror("bind");
        abort();
    }

    while (!done) {
        TSPkt pkt;
        ssize_t bufLen = sizeof(pkt);
        struct sockaddr_in srcAddr;
        socklen_t srcAddrLen = sizeof(srcAddr);
        char srcAddrStr[INET_ADDRSTRLEN];

        // Receive a single packet
        bufLen = recvfrom(fd, (void *)&pkt, (size_t)bufLen, 0,
                       (struct sockaddr *)&srcAddr, &srcAddrLen);
        if (bufLen < 0) {
            perror("recvfrom");
            continue;
        }
        if (bufLen != sizeof(pkt)) {
            printf("Packet recieved with the wrong size!\n");
            continue;
        }

        // Parse Announcement
        processPkt(srcAddr.sin_addr.s_addr, pkt);
        inet_ntop(AF_INET, &srcAddr.sin_addr, srcAddrStr, INET_ADDRSTRLEN);
        printf("Received from %s\n", srcAddrStr);
    }
}
