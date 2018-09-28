
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

#define TIMESYNC_MAGIC  0x1435089464683975

struct TSPkt
{
    uint64_t magic; // Magic
    //uint32_t ip;    // IP Address
    int64_t ts;     // Machine Time
    TSPktMachine machines[32];
};

class TSMachine
{
public:
    TSMachine(uint32_t ip);
    ~TSMachine();
    void addSample(int64_t localts, int64_t remotets);
    uint32_t getIP();
    int64_t getTSDelta();
private:
    uint32_t ip;
    //int64_t tdpeer; // Minimum Time Delta from Peer
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
    void announcer();
    void processPkt(const TSPkt &pkt);
    void listener();
    bool done;
    std::thread *thrAnnounce;
    std::thread *thrSync;
    std::unordered_map<uint32_t,TSMachine> machines;
};

#endif /* __TIMESYNC_H__ */

