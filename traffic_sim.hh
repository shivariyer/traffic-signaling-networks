#ifndef _TRAFFIC_SIM_HH
#define _TRAFFIC_SIM_HH

// This is the simulator for Traffic Signaling Networks

#include <set>
#include <deque>
#include <string>
#include <vector>
#include <utility>
#include <ctime>
#include <cstdio>
#include <cassert>
#include <cstdlib>

#define MAX_EPOCH 1
#define MAX_LINKS 200
#define MAX_DEGREE 20
#define DEBUG 1
#define PRINTF if (DEBUG) printf
#define ERRORPRINT printf

using namespace std;

class Car
{
public:
    int carId;
    deque<int> route;
    float startstamp;
    float timestamp;

    Car(int cid, deque<int> r, float ss, float ts);
    ~Car();
    int get_next_link(int ipLink);
};


class Link
{
public:
    int linkId;
    float cstar;	 // Number of cars per second for optimal flow
    int delay;		 // Free flow delay of the link
    int bmax;		 // Max buffer size
    int bstar;		 // Buffer size for optimal flow
    deque<Car> cars;

    Link(int lid, float c, int del);
    ~Link();
    bool insert_car(Car carToInsert, bool force = false);
    bool delete_car();
    Car* get_front();
    float get_output_capacity();
    int get_delay();
    bool set_capacity(float cstar); // modify the capacity of the link
};


class Source
{
private:
    int sourceId;

public:
    int linkId; // first link out of the source
    int numRoutes;
    int currentTime;
    multiset<int> generationTimes;
    deque<int> vecRoutes[MAX_LINKS];

    Source(int sid, int lid);
    ~Source();
    bool registerBurst(int bStart, int bEnd, int bRate);
    bool registerRoute(deque<int> route);
    bool init();
    bool exec(int cTime);
};


class Sink
{
private:
    int sinkId;

public:
    int linkId; // link into the sink
    int currentTime;

    Sink(int sid, int lid);
    bool exec(int cTime);
};


class Merge
{
private:
    int mergeId;

public:
    int currentTime;
    vector<int> ipLinks;
    int opLink;

    Merge(int mid);
    ~Merge();
    bool isThisOpLink(int lid);
    bool registerIpLink(int lid);
    bool registerOpLink(int lid);
    bool exec(int cTime);
};


class Junction
{
private: 
    int junctionId;
    vector<Link> copyOfLinks;
    
protected:
    int signalMode; // 0: Normal. 1: Equal Proportions.  

public:
    int currentTime;
    vector<int> ipLinks;
    vector<int> opLinks;
    float greenTimes[MAX_DEGREE];
    int currentIpLinkIndex;
    int startTimeForThisLink;

    Junction(int jid, int mode);
    ~Junction();
    bool isThisOpLink(int lid);
    bool registerIpLink(int lid);
    bool registerOpLink(int lid);
    bool exec(int cTime);
    bool find_priority(int ipLink, float& carPriority, int& opLink);
    bool exec_epoch(int cTime);
};

#endif
