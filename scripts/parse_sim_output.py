
# plot capacity of each link, input rate and the exit rate over time

import os
import sys
import pandas as pd
import numpy as np

links = []
sources = []
sinks = []

if len(sys.argv) != 2:
    print('Provide path to sim output file.', file=sys.stderr)
    print('Usage: python {} <file>'.format(sys.argv[0]), file=sys.stderr)
    sys.exit(-1)

simfpath = sys.argv[1]

if not os.path.exists(simfpath):
    print('The specified file {} does not exist!'.format(simfpath))
    sys.exit(-2)

inratefpath = simfpath.replace('sim.out', 'inrates.txt')
exratefpath = simfpath.replace('sim.out', 'exrates.txt')
bufsizesfpath = simfpath.replace('sim.out', 'bufsizes.txt')

with open(sys.argv[1]) as fin:

    # get number of links
    l = fin.readline()
    nlinks = int(l.split()[2])
    print('# links:', nlinks)
    
    # read info about each link
    for ii in range(nlinks+1):
        # Link: linkId 0 cStar 0.000000 delay 0
        l = fin.readline()
        _, _, linkid, _, cstar, _, delay = l.split()
        linkid = int(linkid)
        if linkid == 0:
            # do not include the dummy link
            continue
        cstar, delay = float(cstar), float(delay)
        bstar = cstar * delay
        bmax = bstar * 3
        links.append((linkid, cstar, delay, bstar, bmax))
    print('Links:', links)

    # get number of sources
    l = fin.readline()
    nsources = int(l.split()[2])
    print('# sources:', nsources)

    # read info about each source
    for ii in range(nsources):
        # Source: sourceId 1001 linkId 1. currentTime -1
        while True:
            l = fin.readline()
            if l.startswith('Source'):
                break
        _, _, sourceid, _, linkid = l.split()
        sources.append((int(sourceid), int(linkid)))
    print('Sources:', sources)

    # get number of sinks
    while True:
        l = fin.readline()
        if l.startswith('We'):
            break
    nsinks = int(l.split()[2])
    print('# sinks:', nsinks)

    # read info about each sink
    for ii in range(nsinks):
        # Sink: sinkId 2001 linkId 3
        l = fin.readline()
        _, _, sinkid, _, linkid = l.split()
        sinks.append((int(sinkid), int(linkid)))
    print('Sinks:', sinks)

    # quantities of interest over time
    # - input rate at each source
    # - output rate at each sink
    # - buffer size in each link

    while True:
        l = fin.readline()
        if l.startswith('Total'):
            # Total sim time: 2400 seconds (= 40 minutes)
            totalsimtime = int(l.split()[3])
        elif l.startswith('TIME 0'):
            break

    current_time = 0

    sources_inrate = np.empty((totalsimtime+1, nsources)) * 0
    sinks_exrate = np.empty((totalsimtime+1, nsinks)) * 0
    links_bufsizes = np.empty((totalsimtime+1, nlinks)) * 0
    
    for l in fin:

        if l.startswith('TIME'):
            # next timestamp
            current_time = int(l.split()[1])

        elif l.startswith('linkId'):
            _, linkid, _, bufsize = l.split()
            linkid = int(linkid)
            if linkid > 0:
                links_bufsizes[current_time,linkid-1] = int(bufsize)

        elif l.startswith('exec: sourceId'):
            # this is a new car insertion at the source, so add it to
            # the input rate at that source
            
            # exec: sourceId 1001. time 1. New car carId 1001000001. Route: 1 3
            _, _, sourceid, _, time, _ = l.split(maxsplit=5)
            sources_inrate[int(time[:-1]),int(sourceid[:-1])-1001] += 1

        elif l.startswith('exec: sinkId'):
            # this is car removal at a sink, so add it to the exit
            # rate at that sink

            # exec: sinkId 2001 Removing carId 1001000001 from linkId 3 now 20 starttime 1.0 timestamp 15.0 delay 19.0 overhead 9.0 
            _, _, sinkid, _, _, _, _, _, linkid, _, time, _ = l.split(maxsplit=11)
            sinks_exrate[int(time),int(sinkid)-2001] += 1


    print('Saving input rates at all sources to {}'.format(inratefpath))
    sources_inrate = pd.DataFrame(data=sources_inrate,
                                  index=pd.RangeIndex(0, totalsimtime+1, name='time_s'),
                                  columns=[tup[0] for tup in sources], dtype=int)
    with open(inratefpath, 'w') as fout:
        fout.write('# input rate (number of cars) every second at each source' + os.linesep)
        fout.write('# total sim time: {}, number of sources: {}'.format(totalsimtime, nsources) + os.linesep)
        #np.savetxt(fout, sources_inrate, fmt='%d')
        sources_inrate.to_csv(fout)

    print('Saving exit rates at all sinks to {}'.format(exratefpath))
    sinks_exrate = pd.DataFrame(data=sinks_exrate,
                                index=pd.RangeIndex(0, totalsimtime+1, name='time_s'),
                                columns=[tup[0] for tup in sinks], dtype=int)
    with open(exratefpath, 'w') as fout:
        fout.write('# exit rate (number of cars) every second at each sink' + os.linesep)
        fout.write('# total sim time: {}, number of sinks: {}'.format(totalsimtime, nsinks) + os.linesep)
        #np.savetxt(fout, sinks_exrate, fmt='%d')
        sinks_exrate.to_csv(fout)

    print('Saving all buffer sizes to {}'.format(bufsizesfpath))
    links_bufsizes = pd.DataFrame(data=links_bufsizes,
                                  index=pd.RangeIndex(0, totalsimtime+1, name='time_s'),
                                  columns=[tup[0] for tup in links], dtype=int)
    with open(bufsizesfpath, 'w') as fout:
        fout.write('# buf size (number of cars) every second in each link' + os.linesep)
        fout.write('# total sim time: {}, number of links: {}'.format(totalsimtime, nlinks) + os.linesep)
        #np.savetxt(fout, links_bufsizes, fmt='%d')
        links_bufsizes.to_csv(fout)
