
#include <iostream>
#include <iomanip>
#include <fstream>

#include "traffic_sim.hh"

vector<Link> links;

deque<void*> execPointers;
unsigned int numSources = 0, numSinks = 0, numJunctions = 0, numLinks = 0, numMerges = 0;


void initialize(char* filename)
{
    assert((numSources == 0) && (numSinks == 0) && (numJunctions == 0));
    ifstream fin(filename);
    if (!fin) {
	cout << "Unable to open the parameters file.\n";
	return;
    }

    fin >> numLinks;
    cout << "We have " << numLinks << " links.\n";
    links.push_back(Link(0,0,0)); // This is a dummy link
    for (int i=1; i<=numLinks; i++) {
	// Note that links[0] is a dummy
	int d = 0;
	float cmax = 0.0;
	fin >> cmax;
	fin >> d;
	links.push_back(Link(i,cmax,d));
    }

    fin >> numSources;
    cout << "We have " << numSources << " sources.\n";
    for (int i=0; i<numSources; i++) {
	// Do this for each source that is present.

	int sourceId = 0, linkId = 0, numBursts = 0, numRoutes = 0;
	fin >> sourceId;
	fin >> linkId;
	Source* ptr = new Source(sourceId, linkId);
	execPointers.push_back((void*)(ptr));

	fin >> numBursts;
	for (int j=0; j<numBursts; j++) {
	    int burstStart = 0, burstEnd = 0, burstRate = 0;
	    fin >> burstStart;
	    fin >> burstEnd;
	    fin >> burstRate;
	    assert(ptr->registerBurst(burstStart, burstEnd, burstRate));
        }

	fin >> numRoutes;
	for (int j=0; j<numRoutes; j++) {
	    int routeLength = 0;
	    fin >> routeLength;
	    deque<int> route;
	    route.clear();
	    for (int k=0; k<routeLength; k++) {
		int tmp = 0;
		fin >> tmp;
		route.push_back(tmp);
            }
	    assert(ptr->registerRoute(route));
        }

	assert(ptr->init());
    }

    // Sources are done. Now, we parse the sinks.
    fin >> numSinks;
    cout << "We have " << numSinks << " sinks.\n";
    for (int i=0; i<numSinks; i++) {
	// Do this for each sink that is present.
	int sinkId = 0, linkId = 0;
	fin >> sinkId;
	fin >> linkId;
	Sink* ptr = new Sink(sinkId, linkId);
	execPointers.push_back((void*)(ptr));
    }

    fin >> numJunctions;
    cout << "We have " << numJunctions << " junctions.\n";
    for (int i=0; i<numJunctions; i++) {
	int junctionId = 0, isAllGreen = 0, numIpLinks = 0, numOpLinks = 0;
	fin >> junctionId;
	fin >> isAllGreen;
	Junction* ptr = new Junction(junctionId, isAllGreen);
	execPointers.push_back((void*)(ptr));
	fin >> numIpLinks;
	for (int j=0; j<numIpLinks; j++) {
	    int ipLink;
	    fin >> ipLink;
	    assert(ptr->registerIpLink(ipLink));
        }
	fin >> numOpLinks;
	for (int j=0; j<numOpLinks; j++) {
	    int opLink;
	    fin >> opLink;
	    assert(ptr->registerOpLink(opLink));
        }
    }

    fin >> numMerges;
    cout << "We have " << numMerges << " merges.\n";

    for (int i=0; i<numMerges; i++) {
	int mergeId = 0, numIpLinks = 0;
	fin >> mergeId;

	Merge* ptr = new Merge(mergeId);
	execPointers.push_back((void*)(ptr));

	fin >> numIpLinks;

	for (int j=0; j<numIpLinks; j++) {
	    int ipLink;
	    fin >> ipLink;
	    assert(ptr->registerIpLink(ipLink));
        }

	int opLink = 0;
	fin >> opLink;
	assert(ptr->registerOpLink(opLink));
    }
}


void execute(int totalTime)
{
    assert((numSources + numSinks + numJunctions + numMerges) == execPointers.size());

    PRINTF("\nTotal sim time: %d seconds\n", totalTime);
    
    for (int t=0; t<=totalTime; t++) {
      
	// print some diagnostic information at every timestamp
	PRINTF("\nTIME %d\n", t);
	for (vector<Link>::iterator it = links.begin(); it != links.end(); it++)
	    PRINTF("linkId %d buffer %lu\n", it->linkId, (it->cars).size());

	PRINTF("\n");

	int start = 0;
	for (int i=start; i < start + numSources; i++) {
	    PRINTF("%s: About to call exec on source. Time %d\n", __func__, t);
	    Source* ptr = (Source*) (execPointers[i]);
	    assert(ptr->exec(t));
        }

	start = numSources;
	for (int i=start; i<start+numSinks; i++) {
	    PRINTF("%s: About to call exec on sink. Time %d\n", __func__, t);
	    Sink* ptr = (Sink*) (execPointers[i]);
	    assert(ptr->exec(t));
        }

	start = numSources + numSinks;
	for (int i=start; i<start+numJunctions; i++) {
	    PRINTF("%s: About to call exec on junction. Time %d\n", __func__, t);
	    Junction* ptr = (Junction*) (execPointers[i]);
	    assert(ptr->exec(t));
        }

	start += numJunctions;
	for (int i=start; i<start+numMerges; i++) {
	    PRINTF("%s: About to call exec on merge. Time %d\n", __func__, t);
	    Merge* ptr = (Merge*) (execPointers[i]);
	    assert(ptr->exec(t));
        }
    }
}


int main(int argc, char**argv)
{
    if (argc != 3) {
	printf("Usage: ./a.out parameters_file_name simulation_time\n");
	return 0;
    }

    initialize(argv[1]);   // Set up all the simulation parameters
    execute(atoi(argv[2]));  // Execute the whole damn thing. hmm-kay?
    return 0;
}
