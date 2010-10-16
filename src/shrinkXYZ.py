#!/usr/bin/python

import os
import re

def lidar_processing(workingDir, image, opts):
    pass

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
                image = file.split(".xyz")[0]
            else:
                pass

        lidar_processing(workingDir, image, opts)
    