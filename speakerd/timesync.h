
#ifndef __TIMESYNC_H__
#define __TIMESYNC_H__

#include <thread>

struct TimeSyncMachine
{
    uint32_t ip;    // IP of Other Machine
    int64_t td;     // Minimum Time Delta
};

struct TimeSyncPkt
{
    uint32_t ip;    // IP Address
    int64_t ts;     // Machine Time
    TimeSyncMachine machines[32];
};

class TimeSync
{
public:
    TimeSync();
    ~TimeSync();
    void start();
    void stop();
private:
    void announcer();
    void listener();
    bool done;
    std::thread thrAnnounce;
    std::thread thrSync;
};

#endif /* __TIMESYNC_H__ */

