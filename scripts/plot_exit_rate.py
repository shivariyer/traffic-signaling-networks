import sys
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

# provide as argument the exrates.txt file, which contains raw number
# of cars exiting every sink every second
inpath = sys.argv[1]
#sinks_exrates = np.loadtxt(sys.argv[1], skiprows=2)
sinks_exrates = pd.read_csv(inpath, comment='#', index_col=0)

# window length in seconds
window = 60
window_minutes = window/60

# this is half the window in seconds
radius = (window//2) 

# how often do we want the exit rate report? (seconds)
freq = 10

# calculate avg number of cars per minute in "window", every "freq"
# seconds; do this for each sink separately

savepath_fmt = inpath[:-4] + '_w{:03d}_s{:02d}_sink{{}}'.format(window, freq)
for sinkid in sinks_exrates.columns:

    timestamps = np.arange(1, sinks_exrates[sinkid].size, freq)
    exitrates = np.empty(timestamps.size)
    for ii, ts in enumerate(timestamps):
        start = 1 if ts <= radius else ts - radius
        end = sinks_exrates[sinkid].size - 1 if ts + radius > sinks_exrates[sinkid].size else ts + radius
        exitrates[ii]= sinks_exrates[sinkid].iloc[start:end].sum() / window_minutes

    np.savetxt(savepath_fmt.format(sinkid) + '.txt', np.vstack((timestamps,exitrates)).T, 
               header='time_s,exit_rate', comments='', delimiter=',', fmt=['%.0f', '%.1f'])
    fig = plt.figure()
    ax = fig.add_subplot(111)
    ax.plot(timestamps, exitrates, 'k-o', ms=6)
    ax.grid()
    fig.suptitle('Sink {}'.format(sinkid))
    ax.set_title('Exit rate over a rolling {} min window, computed every {} seconds'.format(window_minutes, freq))
    ax.set_ylabel('Cars per minute')
    ax.set_xlabel('Time (seconds)')
    fig.savefig(savepath_fmt.format(sinkid) + '.png')
    plt.close(fig)
    #plt.show()

print('Saved all the plots.')
