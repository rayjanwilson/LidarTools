#!/usr/bin/python

import os
import re
import numpy as np

def getArray(filename):
    x, y, z = np.loadtxt(filename, delimiter=',', unpack=True)
    data = np.array([x, y, z]).transpose() 
    return data

def printXYZinfo(xyz):
    north_min, north_max, east_min, east_max, elev_min, elev_max = getMinsMaxs(xyz) 
    print "xyz shape: ", xyz.shape
    print "northing: min ", north_min, " max: ", north_max
    print "easting: min ", east_min, " max: ", east_max
    print "elev: min ", elev_min, " max: ", elev_max
    print "northing diff: ", north_max - north_min
    print "easting diff: ", east_max - east_min
    print "elev diff: ", elev_max - elev_min

def getMinsMaxs(xyz):
    north_min = np.min(xyz[:,0])
    north_max = np.max(xyz[:,0])
    east_min = np.min(xyz[:,1])
    east_max = np.max(xyz[:,1])
    elev_min = np.min(xyz[:,2])
    elev_max = np.max(xyz[:,2])
    return north_min, north_max, east_min, east_max, elev_min, elev_max

def saveArray(data, outName='xyz.csv'):
    print "Saving to ", outName
    #np.save(outName, data)
    np.savetxt(outName, data, delimiter=',', fmt='%1.1f')
    


def recastXYZ(xyz):
    """
    The idea here is that much of the data in the XYZ file is redundant. 
    For example, the easting numbers range from 5011997 to 5014002.
    Since there are  4018018 points, the bytes consumed by repeating the 501 over and over again really adds up.
    I propose we have a new header that tells us the base numbers
    We will keep the elevation the same as the original
    """
    north_min, north_max, east_min, east_max, elev_min, elev_max = getMinsMaxs(xyz)
    recast = np.array(np.zeros(xyz.shape), dtype='float32')
    recast[:,0] = xyz[:,0] - north_min
    recast[:,1] = xyz[:,1] - east_min
    recast[:,2] = xyz[:,2] #- elev_min
    print "north max: ", np.max(xyz[:,0])
    print "north min: ", np.min(xyz[:,0])
    print "north max new: ", np.max(recast[:,0])
    print "north min new: ", np.min(recast[:,0])
    print ""
    print "east max: ", np.max(xyz[:,1])
    print "east min: ", np.min(xyz[:,1])
    print "east max new: ", np.max(recast[:,1])
    print "east min new: ", np.min(recast[:,1])
    print ""    
    print "elev max: ", np.max(xyz[:,2])
    print "elev min: ", np.min(xyz[:,2])
    print "elev max new: ", np.max(recast[:,2])
    print "elev min new: ", np.min(recast[:,2])
    
    saveArray(recast)
    

def lidar_processing(image, opts):    
    xyz = getArray(image)
    if opts.debug:
        printXYZinfo(xyz)
    recastXYZ(xyz)
    
    
    

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
    