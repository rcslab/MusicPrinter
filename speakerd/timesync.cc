
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

#include "timesync.h"

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

TimeSync::TimeSync()
{
}

TimeSync::~TimeSync()
{
}

void
TimeSync::start()
{
    done = false;
    thrAnnounce = std::thread(&TimeSync::announcer, this);
    thrSync = std::thread(&TimeSync::listener, this);
}

void
TimeSync::stop()
{
    done = true;

    thrAnnounce.join();
    thrSync.join();
}

void
TimeSync::announcer()
{
    TimeSyncPkt pkt;

    while (!done) {
        pkt.ts = machineTime();

        sleep(1);
    }
}

void
TimeSync::listener()
{
    while (!done) {
        sleep(1);
    }
}

