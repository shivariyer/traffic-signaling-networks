
import sys
import numpy as np
import matplotlib.pyplot as plt

# bmax = 30
# delay = 5
# cstar = 2

# how long does it take for a car to cross the road when it's empty?
# (i.e. like the base latency/delay in networking)
delay = int(sys.argv[1])

# what is the maximum possible exit rate of cars? (i.e. like the
# bandwidth in networking)
cstar = int(sys.argv[2])

# how many cars at max can the road segment hold before the output
# rate begins to drop? (i.e. like the BDP; this would be proportional
# to the width and length of the road segment)
bstar = delay * cstar

# how many cars at max can the road segment hold absolutely? There is
# physically no space for any more beyond this. For buffer sizes
# beyond bstar, the output exit rate begins to drop, and that is the
# traffic curve function, which we will plot here.
bmax = bstar * 3

def output_cap(b):
    if b <= bstar:
        return cstar
    elif bstar < b <= 0.4*bmax:
        return (1.5 - (1.5*b/bmax)) * cstar
    elif 0.4*bmax < b <= 0.6*bmax:
        return (2.2 - (3.25*b/bmax)) * cstar
    elif 0.6*bmax < b <= bmax:
        return (0.475 - (0.375*b/bmax)) * cstar
    else:
        return 0.1 * cstar

if __name__ == '__main__':
    b_list = np.linspace(0, bmax+2, 1000)

    plt.figure()
    x1 = b_list[b_list<=bstar]
    y1 = np.asarray([output_cap(b) for b in x1])
    plt.plot(x1, y1, lw=2)

    x2 = b_list[((bstar<=b_list) & (b_list<=0.4*bmax))]
    y2 = np.asarray([output_cap(b) for b in x2])
    plt.plot(x2, y2, lw=2)

    x3 = b_list[((0.4*bmax<=b_list) & (b_list<=0.6*bmax))]
    y3 = np.asarray([output_cap(b) for b in x3])
    plt.plot(x3, y3, lw=2)

    x4 = b_list[((0.6*bmax<=b_list) & (b_list<=bmax))]
    y4 = np.asarray([output_cap(b) for b in x4])
    plt.plot(x4, y4, lw=2)

    x5 = b_list[b_list>=bmax]
    y5 = np.asarray([output_cap(b) for b in x5])
    plt.plot(x5, y5, lw=2)

    plt.title('Traffic curve')
    plt.show()
    plt.close()
