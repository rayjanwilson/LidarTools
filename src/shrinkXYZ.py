#!/usr/bin/python

import os
import re
import numpy as np

def getArray(filename):
    x, y, z = np.loadtxt(filename, delimiter=',', unpack=True)
    data = np.array([x, y, z]).transpose() 
    return data

def lidar_processing(image, opts):    
    xyz = getArray(image)
    print "xyz shape: ", xyz.shape
    north_min = np.min(xyz[:,0])
    north_max = np.max(xyz[:,0])
    east_min = np.min(xyz[:,1])
    east_max = np.max(xyz[:,1])
    elev_min = np.min(xyz[:,2])
    elev_max = np.max(xyz[:,2])
    print "northing: min ", north_min, " max: ", north_max
    print "easting: min ", east_min, " max: ", east_max
    print "elev: min ", elev_min, " max: ", elev_max 
    

if __name__ == '__main__':
    import optparse
    #parser = optparse.OptionParser(usage='Usage: %prog <options> basefolder', description=__description__, version="%prog version "+__version__)
    parser = optparse.OptionParser(usage='Usage: %prog <options> basefolder')
    parser.add_option(
        '-v', '--verbose',
        dest='verbose',
        action='count',
        help="Increase verbosity (specify multiple times for more)"
    )
    parser.add_option(
        '-d', '--debug',
        dest='debug',
        action='store_true',
        default=False,
        help="runs a smaller image and gives intermediate print statements"
    )
    (opts, args) = parser.parse_args()
    if opts.debug:
        print "opts: ", opts
    if len(args) < 1:
        print "you need to give me a directory"
    else:
        workingDir = args[0]
        os.chdir(workingDir)

        for file in os.listdir("."):
            if re.search('.xyz', file):
                image = file
            else:
                pass

        lidar_processing(image, opts)
    