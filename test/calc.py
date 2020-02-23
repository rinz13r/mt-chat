import sys
import math

if __name__ == '__main__':
    tot, n = 0, 0
    while (True):
        x = input ();
        if (x == "end") : break
        if (len (x) == 0) : continue
        if (math.isnan (float (x))) :
            continue
        tot += float (x)
        n += 1

    try:
        print ("[%f]us" %(tot/n))
    except Exception as e:
        print ((n, tot))
