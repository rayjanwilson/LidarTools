#!/usr/bin/python

import os
import json
import numpy as np


def getArray(filename):
    data=[]
    x, y, z = np.loadtxt(filename, delimiter=' ', unpack=True)
    #print x,y,z
    data = np.array([x, y, z]).transpose()
    return data

def saveArray(data, outName='Data/uaf.json'):
    print "Saving to ", outName
    #np.save(outName, data)
    np.savetxt(outName, data, delimiter=',', fmt='%1.1f')


xyz = getArray("Data/uaf-campus2.txt")
saveArray(xyz)


# sed -e 's/$/,/g' -e '$ s/,$//' uaf.json > uaf.json.new && mv uaf.json.new uaf.json
# then vi G for [ ]
