
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

#include <netinet/in.h>
#include <netinet/tcp.h>

#include "timesync.h"

using namespace std;

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
TSMachine::addSample(int64_t localts, int64_t remotets)
{
    ts.push_front(abs(localts - remotets));
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

TimeSync::TimeSync() : done(false), thrAnnounce(nullptr), thrSync(nullptr)
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
    dstAddr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
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

        printf("Announcement Sent");

        sleep(1);
    }
}

void
TimeSync::processPkt(const TSPkt &pkt)
{
}

void
TimeSync::listener()
{
    int fd;
    int status;
    struct sockaddr_in addr;
    int reuseaddr = 1;

    fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (fd < 0) {
        perror("socket");
        abort();
    }

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

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(TIMESYNC_PORT);

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

        bufLen = recvfrom(fd, (void *)&pkt, (size_t)bufLen, 0,
                       (struct sockaddr *)&srcAddr, &srcAddrLen);
        if (bufLen < 0) {
            perror("recvfrom");
            continue;
        }
        if (bufLen != sizeof(pkt)) {
            perror("Packet recieved with the wrong size!");
            continue;
        }

	// Parse Announcement
        printf("Received");
        processPkt(pkt);
    }
}

