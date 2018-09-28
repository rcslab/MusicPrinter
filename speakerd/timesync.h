
#ifndef __TIMESYNC_H__
#define __TIMESYNC_H__

#include <list>
#include <unordered_map>
#include <thread>

struct TSPktMachine
{
    uint32_t ip;    // IP of Other Machine
    int64_t td;     // Minimum Time Delta
};

#define TIMESYNC_MAGIC      0x1435089464683975
#define TIMESYNC_MACHINES   32

struct TSPkt
{
    uint64_t magic; // Magic
    //uint32_t ip;    // IP Address
    int64_t ts;     // Machine Time
    TSPktMachine machines[TIMESYNC_MACHINES];
};

class TSMachine
{
public:
    TSMachine() {}
    TSMachine(const TSMachine &t) = default;
    TSMachine(uint32_t ip);
    ~TSMachine();
    void dump();
    void addSample(int64_t localts, int64_t remotets);
    uint32_t getIP();
    int64_t getTSDelta();
    int64_t tdpeer; // Minimum Time Delta from Peer
private:
    uint32_t ip;
    std::list<int64_t> ts; // Time Deltas
};

class TimeSync
{
public:
    TimeSync();
    ~TimeSync();
    void start();
    void stop();
    int64_t getTime();
    void sleepUntil(int64_t ts);
private:
    void dump();
    void announcer();
    void processPkt(uint32_t src, const TSPkt &pkt);
    void listener();
    bool done;
    uint32_t myIP;
    std::thread *thrAnnounce;
    std::thread *thrSync;
    std::unordered_map<uint32_t,TSMachine> machines;
};

#endif /* __TIMESYNC_H__ */

