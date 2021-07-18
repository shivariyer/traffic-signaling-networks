// This is the simulator for Traffic Signaling Networks

#include "traffic_sim.hh"

extern vector<Link> links;

Car::Car(int cid, deque<int> r, float ss, float ts) :
    carId(cid), route(r), startstamp(ss), timestamp(ts)
{
    PRINTF("%s: Creating a car. carId %d timestamp %f. Route: ",
	   __func__, carId, timestamp);
    for (deque<int>::iterator it = route.begin(); 
	 it != route.end(); it++)
	PRINTF("%d ", *it);
    PRINTF("\n");
}

Car::~Car()
{
    route.clear();
}

int Car::get_next_link(int ipLink)
{
    PRINTF("%s: carId %d ipLink %d ", __func__, carId, ipLink);

    int opLink;
    for (int i=0; i<route.size(); i++) {
	if (route[i] == ipLink) {
	    assert ((i+1) < route.size());
	    opLink = route[i+1];
	    PRINTF("ipLInk %d opLink %d\n", ipLink, opLink);
	    return opLink;
	}
    }

    // Control should not come here.
    assert(false);
    return -1;
}


Link::Link(int lid, float c, int del) :
    linkId(lid), cstar(c), delay(del)
{
    cars.clear();
    bstar = cstar * delay; // "bandwidth-delay product"
    bmax = bstar * 3;
    PRINTF("%s: linkId %d cStar %f delay %d\n",
	   __func__, linkId, cstar, delay);
}

Link::~Link()
{
    cars.clear();
}

bool Link::insert_car(Car carToInsert, bool force)
{
    // Only the source is allowed to "force" a car insertion,
    // ignoring the link capacity.

    assert(linkId > 0);
    bool valid = false;

    // As an additional safety precaution, make sure that this
    // link is in the route for the car.
    for (int i=0; i<carToInsert.route.size(); i++)
	if (carToInsert.route[i] == linkId) {
	    valid = true;
	    if (force)
		assert(i == 0);
	}

    if (!valid) {
	printf("\nlinkId: %d. carId: %d Car route: ", linkId, carToInsert.carId);
	for (int a=0; a<carToInsert.route.size(); a++)
	    printf("%d ", carToInsert.route[a]);
    }
    assert(valid);

    if (force && (cars.size() > bstar)) {
	// The source is trying to insert a car, but the link
	// already holds more than bstar. In this case, *drop*
	// the car.

	ERRORPRINT("%s: Dropping car: linkId %d carId %d car timestamp %f, startstamp %f qsize %d bstar %d\n",
		   __func__, linkId, carToInsert.carId,
		   carToInsert.timestamp, carToInsert.startstamp,
		   (int) cars.size(), bstar);
	return true;
    }

    if (cars.size() < bmax) {
	PRINTF("%s: linkId %d carId %d car timestamp %f startstamp %f qsize %d\n",
	       __func__, linkId, carToInsert.carId,
	       carToInsert.timestamp, carToInsert.startstamp, (int)cars.size());

	cars.push_front(carToInsert);
	return true;
    }

    if (!force) {
	ERRORPRINT("%s: ERROR. linkId %d carId %d timestamp %f bmax %d num_cars %d. Caused by an allGreen signal.\n",
		   __func__, linkId, carToInsert.carId, carToInsert.timestamp,
		   bmax, (int)cars.size());
	return false;
    }

    cars.push_front(carToInsert);
    return true;
}

bool Link::delete_car()
{
    int n = cars.size();
    if (n == 0) {
	ERRORPRINT("%s: ERROR. linkId %d\n", __func__, linkId);
	return false;
    }

    PRINTF("%s: linkId %d Deleting carId %d timestamp %f qsize %d\n",
	   __func__, linkId, cars[cars.size()-1].carId,
	   cars[cars.size()-1].timestamp, (int)cars.size());
    cars.pop_back();
    return true;
}

Car* Link::get_front()
{
    int n = cars.size();
    if (n == 0) {
	//PRINTF("%s: linkId %d is empty\n", __func__, linkId);
	return NULL;
    } else {
	//PRINTF("%s: linkId %d carId %d timestamp %f\n",
	//        __func__, linkId, cars[n-1].carId, cars[n-1].timestamp);
	return &(cars[n-1]);
    }
}

float Link::get_output_capacity()
{
    PRINTF("%s: linkId %d: b %d bmax %d, cstar %f capacity ",
	   __func__, linkId, (int)(cars.size()), bmax, cstar);
    int b = cars.size();
    float m, y;

    if (b <= bstar) 
	y = cstar;
    else if (b <= 0.4*bmax) {
	m = (-3.0*cstar/2.0) / bmax;
	y = (b - bstar)*m + cstar;
    } else if (b <= 0.6*bmax) {
	m = (-3.25*cstar) / bmax;
	y = (b - bmax*0.4)*m + (cstar*0.9);
    } else if (b <= bmax) {
	m = (-0.375*cstar) / bmax;
	y = (b - bmax*0.6)*m + (cstar*0.25);
    } else
	y = 0.1*cstar;

    PRINTF("%f\n", y);
    return y;
}

int Link::get_delay()
{
    float opCapacity = get_output_capacity();
    float finalDelay = float(delay) * cstar / opCapacity;
    PRINTF("%s: linkId %d, base delay %d, output capacity %f, final delay %d\n",
	   __func__, linkId, delay, opCapacity, (int)(finalDelay));
    //return (int)(delay);

    // Simply return the base delay of the link.
    return delay;
}


Source::Source(int sid, int lid) :
    sourceId(sid), linkId(lid), numRoutes(0), currentTime(-1)
{
    generationTimes.clear();
    PRINTF("%s: sourceId %d linkId %d\n",
	   __func__, sourceId, linkId);
}

Source::~Source()
{
    generationTimes.clear();
    numRoutes = 0;
}

bool Source::registerBurst(int bStart, int bEnd, int bRate)
{
    // bStart and bEnd are start and end timestamps in
    // seconds, and bRate is the rate in cars per minute,
    // that's why there is a division by 60.0

    PRINTF("%s: start %d end %d rate %d\n",
	   __func__, bStart, bEnd, bRate);
    int range = bEnd - bStart;
    float numCars = bRate * (range/60.0);
    int carTime;

    // Random.
    /*for (int i=0; i<numCars; i++)
      {
      int carTime = rand() % range;
      carTime += bStart;
      generationTimes.insert(carTime);
      }*/

    // Uniform Distribution of cars.
    for (float t = float(bStart); t < float(bEnd); t += range/numCars) {
	carTime = int(t);
	generationTimes.insert(carTime);
    }

    return true;
}

bool Source::registerRoute(deque<int> route)
{
    if (route[0] != linkId) {
	PRINTF("The first link of this route is %d. Expected: %d\n", route[0], linkId);
	return false;
    }

    PRINTF("%s: Route is: ", __func__);

    for (int i=0; i<route.size(); i++)
	PRINTF("%d ", route[i]);
    PRINTF("\n");
    vecRoutes[numRoutes++] = route;
    return true;
}

bool Source::init()
{
    // Use this later if needed.
    return true;
}

bool Source::exec(int cTime)
{
    static int cid = 1;

    // this function must be called at every timestamp, if not
    // then it's an error
    if ((cTime - currentTime) != 1) {
	PRINTF("%s: sourceId %d. Time discrepancy. Current Time %d. Got %d\n",
	       __func__, sourceId, currentTime, cTime);
	return false;
    }

    //PRINTF("%s: sourceId %d. Current time updated from %d to %d\n",
    //        __func__, sourceId, currentTime, cTime);

    // generate as many cars as the number of times "cTime"
    // appears in the generationTimes set
    currentTime = cTime;
    multiset<int>::iterator it;
    pair<multiset<int>::iterator, multiset<int>::iterator> ret;
    ret = generationTimes.equal_range(cTime);

    for (it=ret.first; it!=ret.second; it++) {
	// Generate a car
	int id = sourceId*1000000 + cid;
	cid++;
	assert(numRoutes > 0);
	deque<int> r = vecRoutes[(rand() % numRoutes)];
	int ts = links[linkId].get_delay() + cTime;

	PRINTF("%s: sourceId %d. time %d. New car carId %d. Route: ",
	       __func__, sourceId, cTime, id);
	for (int i=0; i<r.size(); i++)
	    PRINTF("%d ", r[i]);
	PRINTF("\n");

	// The "true" is to force insertions.
	assert(links[linkId].insert_car(Car(id, r, cTime, ts), true));
    }

    generationTimes.erase(cTime);

    return true;
}


Sink::Sink(int sid, int lid):
    sinkId(sid), linkId(lid), currentTime(-1)
{
    PRINTF("%s: sinkId %d linkId %d\n", __func__, sinkId, linkId);
}

bool Sink::exec(int cTime)
{
    if ((cTime - currentTime) != 1) {
	ERRORPRINT("%s: sinkId %d. Time discrepancy. Expected %d. Got %d\n",
		   __func__, sinkId, currentTime+1, cTime);
	return false;
    }

    currentTime = cTime;

    if ((cTime % MAX_EPOCH) != 0)
	return true;

    for (float t = 0; t < MAX_EPOCH; ) {
	Car* carPtr = links[linkId].get_front();
	if (carPtr == NULL)
	    break;

	if (carPtr->timestamp > (float)(t) + ((float)(cTime))) {
	    t += 0.1;
	    continue;
	}

	// Control comes here if we have found a car to deque.
	float idealTime = 0.0;
	for (deque<int>::iterator it = carPtr->route.begin(); it != carPtr->route.end(); it++)
	    idealTime += (float)(links[(*it)].get_delay());

	printf("%s: sinkId %d Removing carId %d from linkId %d now %d starttime %f timestamp %f delay %f overhead %f \n", __func__, sinkId, carPtr->carId, linkId, cTime, carPtr->startstamp, carPtr->timestamp, cTime - carPtr->startstamp, (cTime - carPtr->startstamp)-idealTime);

	float c = links[linkId].get_output_capacity();
	assert(links[linkId].delete_car());
	t += (float)(1.0/c);
    }

    return true;
}


Merge::Merge(int mid):
    mergeId(mid), currentTime(-1), opLink(0)
{
    ipLinks.clear();
    PRINTF("%s: mergeId %d\n", __func__, mergeId);
}

Merge::~Merge()
{
    ipLinks.clear();
}

bool Merge::isThisOpLink(int lid)
{
    if (lid == opLink)
	return true;
    else
	return false;
}

bool Merge::registerIpLink(int lid)
{
    ipLinks.push_back(lid);
    PRINTF("%s: mergeId %d linkId %d\n",
	   __func__, mergeId, lid);
    return true;
}

bool Merge::registerOpLink(int lid)
{
    assert(opLink == 0);
    opLink = lid;
    PRINTF("%s: mergeId %d linkId %d\n",
	   __func__, mergeId, lid);
    return true;
}

bool Merge::exec(int cTime)
{
    if ((cTime - currentTime) != 1) {
	ERRORPRINT("Time discrepancy. currentTime %d. Got %d\n", currentTime, cTime);
	return false;
    }

    currentTime = cTime;

    if ((cTime % MAX_EPOCH) != 0)
	return true;

    // Now, we merge 1 epoch worth of cars from linkToSignal to the single output link.

    Car* carToMove = NULL;
    int ipLinkToUse = 0;

    for (float t = 0; t < MAX_EPOCH; ) {
	carToMove = NULL;
	ipLinkToUse = 0;

	for (int i=0; i<ipLinks.size(); i++) {
	    Car* carPtr = links[ipLinks[i]].get_front();
	    if (carPtr == NULL)
		continue;

	    if (carPtr->timestamp > (float)(t) + ((float)(cTime)))
		continue;

	    if (!carToMove) {
		ipLinkToUse = links[ipLinks[i]].linkId;
		assert(ipLinkToUse == ipLinks[i]);
		carToMove = carPtr;
	    }

	    if ((carToMove) && (carToMove->timestamp > carPtr->timestamp)) {
		ipLinkToUse = links[ipLinks[i]].linkId;
		assert(ipLinkToUse == ipLinks[i]);
		carToMove = carPtr;
	    }
	}

	if (carToMove == NULL) {
	    t += 0.1;
	    continue;
	}

	// We have found a car to move.

	int opl = carToMove->get_next_link(ipLinkToUse);
	assert(opl > 0);
	assert(isThisOpLink(opl));

	if (links[opl].cars.size() >= links[opl].bmax) {
	    // In this case, there is no space in the output link to put in
	    // a new car. Give some more time and try again.
	    t += 0.1;
	    continue;
	}

	float c = 0.0;
	c = links[ipLinkToUse].get_output_capacity();

	float delay = links[opl].get_delay();
	carToMove->timestamp = cTime + t + (int)(delay);
	PRINTF("%s: About to merge carId %d new timestamp %f from link %d to link %d. Time %f\n",
	       __func__, carToMove->carId, carToMove->timestamp, ipLinkToUse, opl, (float)(cTime) + t);
	assert(links[opl].insert_car(*carToMove));
	assert(links[ipLinkToUse].delete_car());

	t += (float)(1.0/(c*ipLinks.size()));
	// This works only if all incoming links have
	// the same cstar. Make sure the parameters.txt
	// file satisfies this condition.
    }
    return true;
}


Junction::Junction(int jid, int mode):
    junctionId(jid), currentTime(-1), signalMode(mode), currentIpLinkIndex(0), startTimeForThisLink(0)
{
    ipLinks.clear();
    opLinks.clear();
    assert((mode>=0) && (mode<=2));
    PRINTF("%s: junctionId %d signalMode %d\n", __func__, junctionId, signalMode);
}

Junction::~Junction()
{
    ipLinks.clear();
    opLinks.clear();
}

bool Junction::isThisOpLink(int lid)
{
    for (int i=0; i< opLinks.size(); i++) {
	int l = opLinks[i];
	if (l == lid)
	    return true;
    }
    return false;
}

bool Junction::registerIpLink(int lid)
{
    ipLinks.push_back(lid);
    PRINTF("%s: junctionId %d linkId %d\n",
	   __func__, junctionId, lid);
    return true;
}

bool Junction::registerOpLink(int lid)
{
    opLinks.push_back(lid);
    PRINTF("%s: junctionId %d, linkId %d\n",
	   __func__, junctionId, lid);
    return true;
}

bool Junction::exec(int cTime)
{
    int linkToSignal = 0;

    if ((cTime - currentTime) != 1) {
	ERRORPRINT("Time discrepancy. currentTime %d. Got %d\n", currentTime, cTime);
	return false;
    }

    currentTime = cTime;

    if (cTime % MAX_EPOCH == 0) {
	PRINTF("junctionId %d:%s: cTime %d. We are at an epoch boundary.\n", junctionId, __func__, cTime);
	assert(exec_epoch(cTime));
	currentIpLinkIndex = 0;
	linkToSignal = ipLinks[currentIpLinkIndex];
	startTimeForThisLink = cTime;
    }

    while (startTimeForThisLink + greenTimes[ipLinks[currentIpLinkIndex]] == cTime)  {
	// Time to change the signal to the next input link.
	if (currentIpLinkIndex == ipLinks.size() - 1) {
	    // That's all links we have now.
	    linkToSignal = 0;

	    PRINTF("%s: junctionId %d. This is the last link %d we had to schedule in this epoch.\n",
		   __func__, junctionId, links[currentIpLinkIndex].linkId);
	    break;
	} else {
	    PRINTF("%s: junctionId %d. Switching from link %d to link %d.\n",
		   __func__, junctionId, links[currentIpLinkIndex].linkId, links[currentIpLinkIndex+1].linkId);
	    // Switch to the next signal
	    currentIpLinkIndex++;
	    linkToSignal = ipLinks[currentIpLinkIndex];
	    startTimeForThisLink = cTime;
	}
    }

    // Now, we move 1 epoch worth of cars from linkToSignal to the appropriate output link(s).
    if ((startTimeForThisLink == cTime) && (linkToSignal != 0)) {
	// This green signal has just started. Now, put all cars in the
	// appropriate output queues.

	for (float t = 0; t < greenTimes[linkToSignal]; ) {
	    // 1. Check to see if the car is eligible
	    Car* ptrCar = links[linkToSignal].get_front();
	    if (ptrCar == NULL) {
		t = greenTimes[linkToSignal];
		break;
	    }

	    if (ptrCar->timestamp > startTimeForThisLink + t) {
		// The car is not yet eligible
		t += 0.1;
		continue;
	    }

	    // Car is eligible
	    int opl = ptrCar->get_next_link(linkToSignal);
	    assert(opl > 0);

	    float c = 0.0;
	    c = links[linkToSignal].get_output_capacity();

	    //float delay = (float)(links[opl].d) * links[opl].cstar / links[opl].get_output_capacity();
	    float delay = links[opl].get_delay();
	    ptrCar->timestamp = startTimeForThisLink + t + (int)(delay);

	    // When we have an "equal proportions" signal, the destination link can be full, but the
	    // junction will still try to force a car to be inserted into it. As a result, we should
	    // prevent it.
	    if (links[opl].insert_car(*ptrCar))
		assert(links[linkToSignal].delete_car());
	    else
		assert(signalMode == 1);

	    t += (float)(1.0/c);
	}
    }
    return true;
}

bool Junction::find_priority(int ipLink, float& carPriority, int& opLink)
{
    bool retVal = false;
    Car* carPtr = copyOfLinks[ipLink].get_front();
    for (int i=0; i < carPtr->route.size(); i++) {
	if (carPtr->route[i] == ipLink) {
	    retVal = true;
	    assert ((i+1) < carPtr->route.size());
	    assert(isThisOpLink(carPtr->route[i+1]));
	    opLink = carPtr->route[i+1];
	    if (copyOfLinks[opLink].cars.size() > copyOfLinks[opLink].bstar) {
		// We should not add any more traffic to this already jammed outgoing link.
		carPriority = -1.0;
		return true;
	    }
	    float ipDensity = (float)(copyOfLinks[ipLink].cars.size())/(float)(copyOfLinks[ipLink].bmax);
	    float opDensity = (float)(copyOfLinks[opLink].cars.size())/(float)(copyOfLinks[opLink].bmax);
	    carPriority = ipDensity - opDensity;
	    if (carPriority == -1.0)
		carPriority += 0.001; // Some corner case checking.
	    return true;
	}
    }
}

bool Junction::exec_epoch(int cTime)
{
    int IPlinkToUse = 0;
    int OPlinkToUse = 0;

    // Operate on a temporary copy, because this is the decision making phase and not the real transfer.
    copyOfLinks = links;
    float te = 0;
    for (int i=0; i<MAX_DEGREE; i++)
	greenTimes[i] = 0.0;

    while (te < MAX_EPOCH) {
	float maxPriority = -1.0;
	float carPriority = -1.0;
	int opl = 0;
	for (int i=0; i<ipLinks.size(); i++) {
	    int ipl = ipLinks[i];
	    Car* ptrCar = copyOfLinks[ipl].get_front();
	    if (ptrCar == NULL)
		continue;

	    if (ptrCar->timestamp > cTime)
		continue; // We care about those cars that are already eligible.

	    // Control comes here if we have found an eligible car.
	    carPriority = -1.0;
	    assert(find_priority(ipl, carPriority, opl)); // val, alias, alias
	    PRINTF("junctionId %d: %s: carId %d carPriority %f opLink %d\n", junctionId,
		   __func__, ptrCar->carId, carPriority, copyOfLinks[opl].linkId);
	    if (carPriority > maxPriority) {
		maxPriority = carPriority;
		IPlinkToUse = ipl;
		OPlinkToUse = opl;
	    }
	} // i loop for i/p links

	if (maxPriority == -1)
	    break; // There is no car to schedule here.

	// Control comes here if there is a car to schedule.
	// Move a car from link "IPlinkToUse" to "OPlinkToUse";

	PRINTF("junctionId %d: %s: Car to move: carId %d ipLink %d opLink %d carPriority %f\n",
	       junctionId, __func__, copyOfLinks[IPlinkToUse].get_front()->carId, IPlinkToUse, copyOfLinks[OPlinkToUse].linkId, maxPriority);

	assert(copyOfLinks[OPlinkToUse].insert_car(*(copyOfLinks[IPlinkToUse].get_front())));
	assert(copyOfLinks[IPlinkToUse].delete_car());

	float c;
	c = copyOfLinks[IPlinkToUse].get_output_capacity();

	te += (float)(1.0/c);
	greenTimes[IPlinkToUse] += (float)(1.0/c);
    } // while (te < MAX_EPOCH)

    if (signalMode == 1) {
	PRINTF("%s: Override everything. Set signals to equal proportions.\n", __func__);
	// Equal proportions. Override the previous calculations and equally divide.
	for (int i=0; i<ipLinks.size(); i++) {
	    IPlinkToUse = ipLinks[i];
	    greenTimes[IPlinkToUse] = MAX_EPOCH / ipLinks.size();
	}
    }

    PRINTF("%s: junctionId %d. The schedule for the upcoming epoch is: ",
	   __func__, junctionId);
    for (int i=0; i<ipLinks.size(); i++)
	PRINTF("%f ", greenTimes[ipLinks[i]]);
    PRINTF("\n");

    return true;
}
