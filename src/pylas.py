#!/bin/env python
"""
 pylas 0.1

 Description:
   Reads Light Detection and Ranging (LIDAR) data in binary LAS format
   and converts to GIS point data formats (xyz or shapefile)
 
 Reference:
   ASPRS LIDAR Data Exchange Format Standard Version 1.1 March 7, 2005
   http://www.lasformat.org/documents/ASPRS%20LAS%20Format%20Documentation%20-%20V1.1%20-%2003.07.05.pdf
 
 Authors:
   Matthew Perry 
   Carl Anderson

 Last Modified:
   12/18/2006
 
 License:
 "MIT License"
 Copyright (c) 2007 Carl Anderson, Matthew Perry 

 Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

  Changelog:
   12/18/06. Carl Anderson's updates once again
    - added getopt support and refactored parameter input.
    - merged the file reads to gain some performance improvements.
    - ascii xyz output support
    - Understands both 1.0 and 1.1 LAS binary structure definitions
    - filter out first or last returned pulses
    - outlined some potential future parameters for field selection
   11/06/06. Based on changes submitted by Carl Anderson
    - corrected the LAS object structure  (some things are arrays of byte not chars)
    - changed the output to be 3D points
    - added a parameter to control the number of points processed
"""
import sys, os, struct, getopt, string

#  LAS header binary structure:
#  (Description, bytes, struct type, array size)
headerstruct = ( ('filesig', 4,'c',4) ,
               ('filesourceid' , 2,'H',1) ,
               ('reserved'     , 2,'H',1) ,
               ('guid1'        , 4,'L',1) ,
               ('guid2'        , 2,'H',1) ,
               ('guid3'        , 2,'H',1) ,
               ('guid4'        , 8,'B',8) ,
               ('vermajor'     , 1,'B',1) ,
               ('verminor'     , 1,'B',1) ,
               ('sysid'        , 32,'c',32) ,
               ('gensoftware'  , 32,'c',32) ,
               ('fileday'      , 2,'H',1) ,
               ('fileyear'     , 2,'H',1) ,
               ('headersize'   , 2,'H',1) ,
               ('offset'       , 4,'L',1) ,
               ('numvlrecords' , 4,'L',1) ,
               ('pointformat'  , 1,'B',1) ,
               ('pointreclen'  , 2,'H',1) ,
               ('numptrecords' , 4,'L',1) ,
               ('numptbyreturn', 20,'L',5) ,
               ('xscale'       , 8,'d',1) ,
               ('yscale'       , 8,'d',1) ,
               ('zscale'       , 8,'d',1) ,
               ('xoffset'      , 8,'d',1) ,
               ('yoffset'      , 8,'d',1) ,
               ('zoffset'      , 8,'d',1) ,
               ('xmax'         , 8,'d',1) ,
               ('xmin'         , 8,'d',1) ,
               ('ymax'         , 8,'d',1) ,
               ('ymin'         , 8,'d',1) ,
               ('zmax'         , 8,'d',1) ,
               ('zmin'         , 8,'d',1) )

#  LAS Point Data Record Format 0:  (20 bytes)
point_fmt0struct = ( ('x', 4,'L',1) ,
                   ('y', 4,'L',1) ,
                   ('z', 4,'L',1) ,
                   ('intensity', 2,'H',1) ,
                   ('return_grp', 1,'B',1) ,
                   ('classification', 1,'B',1) ,
                   ('scan_angle_rank', 1,'B',1) ,
                   ('extra_1', 1,'B',1) ,  # definition but not size changes between LAS 1.0 and LAS 1.1
                   ('extra_2', 2,'H',1)   # definition but not size changes between LAS 1.0 and LAS 1.1
)

#  LAS Point Data Record Format 1:   (28 bytes)
point_fmt1struct = ( ('x', 4,'L',1) ,
                   ('y', 4,'L',1) ,
                   ('z', 4,'L',1) ,
                   ('intensity', 2,'H',1) ,
                   ('return_grp', 1,'B',1) ,
                   ('classification', 1,'B',1) ,
                   ('scan_angle_rank', 1,'B',1) ,
                   ('extra_1', 1,'B',1) ,  # definition but not size changes between LAS 1.0 and LAS 1.1
                   ('extra_2', 2,'H',1) ,  # definition but not size changes between LAS 1.0 and LAS 1.1
                   ('gps_time', 8, 'd', 1) 
)


def parseHeader(filename,verbose=False):
    fh = open(filename,'rb')

    header = {'infile':filename}

    for i in headerstruct:
        try:
            if i[2] == 'c':
                # If it's a string, keep it
                value = fh.read(i[1])
            elif i[3] > 1:
                # If it's an array, unpack to a tuple
                value = struct.unpack( str(i[3]) + i[2] , fh.read(i[1]) ) 
            else:
                # If it's a single binary number
                # Grab the first and only value in the tuple
                value = struct.unpack( i[2] , fh.read(i[1]) )[0]
        except:
            value = fh.read(i[1])

        if verbose:
            print i[0] + '\t', value

        header[i[0]] = value

    fh.close()
    return header

def createShp(outfile, h, numpts=None, rand=True, dim=3, sample=1):

    # Create the Shapefile
    driver = ogr.GetDriverByName('ESRI Shapefile')
    if os.path.exists(outfile):
        driver.DeleteDataSource(outfile)
    ds = driver.CreateDataSource(outfile)

    # Is shp to be 3 dimensional or not
    if dim == 3:
        layer = ds.CreateLayer(outfile, geom_type=ogr.wkbPoint25D)
    else:
        layer = ds.CreateLayer(outfile, geom_type=ogr.wkbPoint)

    ## TODO: base Width and Precision on metadata
    fd = ogr.FieldDefn('elev', ogr.OFTReal)
    w = max([len(str(h['zmax'])),len(str(h['zmin']))])
    #compute the most decimal places and then account for the '0.' part
    p = max([len(str(h['xscale']-int(h['xscale']))),len(str(h['yscale']-int(h['yscale'])))])-2
    fd.SetWidth(w)
    fd.SetPrecision(p)
    layer.CreateField(fd)   
    fd = ogr.FieldDefn('return_num', ogr.OFTReal)
    fd.SetWidth(1)
    fd.SetPrecision(0)
    layer.CreateField(fd)   
    fd = ogr.FieldDefn('angle', ogr.OFTReal)
    fd.SetWidth(3)
    fd.SetPrecision(0)
    layer.CreateField(fd)   
    fd = ogr.FieldDefn('intensity', ogr.OFTReal)
    fd.SetWidth(3)
    fd.SetPrecision(0)
    layer.CreateField(fd)   
    fd = ogr.FieldDefn('asprsclass', ogr.OFTReal)
    fd.SetWidth(3)
    fd.SetPrecision(0)
    layer.CreateField(fd)   
    fd = ogr.FieldDefn('return_tot', ogr.OFTReal)
    fd.SetWidth(3)
    fd.SetPrecision(0)
    layer.CreateField(fd)
    fd = ogr.FieldDefn('gps_time', ogr.OFTReal)
    fd.SetWidth(13)
    fd.SetPrecision(6)
    layer.CreateField(fd)

    if rand: 
        import random
        if numpts is None:
            # Calculate reasonable number of pts (1/2000th of total?)
            # Eventually base on output resolution of DEM and extents
            numpts = h['numptrecords'] / 2000
        if verbose:
            print 'Reading ' + str(numpts) + ' random features from \n   ' + h['infile']
    else:
        if numpts is None:
           numpts = h['numptrecords']
        if verbose:
           print 'Reading first ' + str(numpts) + ' features from \n   ' + h['infile']

    # Open the LAS file and skip the header
    fh = open(h['infile'],'rb',8192)
    fh.seek(h['offset'])
    f= ogr.Feature(feature_def=layer.GetLayerDefn())

    for i in range(0,numpts,sample):
        """
        xrecord = struct.unpack('L',fh.read(4))[0]
        yrecord = struct.unpack('L',fh.read(4))[0]
        zrecord = struct.unpack('L',fh.read(4))[0]
        intensity = struct.unpack('H',fh.read(2))[0]
        return_grp = struct.unpack('B',fh.read(1))[0]
        classification = struct.unpack('B',fh.read(1))[0]
        scan_angle = struct.unpack('b',fh.read(1))[0]
        file_marker = struct.unpack('b',fh.read(1))[0]
        user_fld = struct.unpack('H',fh.read(2))[0]
        gps_time = struct.unpack('d',fh.read(8))[0]
        """

        # assuming format 1
        xrecord,yrecord,zrecord,intensity,return_grp,classification, \
            scan_angle, file_marker, user_fld, gps_time = \
            struct.unpack('LLLHBBbbHd',fh.read(28))

        # format 0
        """
        xrecord,yrecord,zrecord,intensity,return_grp,classification, \
            scan_angle, file_marker, user_fld = \
            struct.unpack('LLLHBBbbH',fh.read(20))
        """

        return_num = return_grp & 7
        return_tot = (return_grp >> 3) &7
        scan_dir = (return_grp >> 6) & 1
        scan_edge = (return_grp >> 7)

        # print "rec:",xrecord,yrecord,zrecord,return_num,return_tot,intensity,scan_angle,file_marker,user_bit,gps_time
        x = (xrecord * h['xscale'] ) + h['xoffset']
        y = (yrecord * h['yscale'] ) + h['yoffset']
        z = (zrecord * h['zscale'] ) + h['zoffset']

        # Create the feature if the values are reasonable
        if h['zmin'] <= z <= h['zmax'] and \
           h['xmin'] <= x <= h['xmax'] and \
           h['ymin'] <= y <= h['ymax']: 

            # Fill in the elevation field
            f.SetField(0, z)
            f.SetField(1, return_num)
            f.SetField(2, scan_angle)
            f.SetField(3, intensity)
            f.SetField(4, classification)
            f.SetField(5, return_tot)
            f.SetField(6, gps_time)

            # Create the point feature
            if dim == 3:
                wkt = 'POINT(%f %f %f)' % (x, y, z)
            else:
                wkt = 'POINT(%f %f)' % (x,y)

            g = ogr.CreateGeometryFromWkt(wkt)
            f.SetGeometryDirectly(g)
            layer.CreateFeature(f)
        else:
            print "Point excluded. x,y,z is out of range: ", x,y,z

        # Move it along
        if rand:
            # Pick a random point record and seek to it's position
            rp = random.randint(0,h['numptrecords']-1)
            fh.seek( h['offset'] + ( rp * h['pointreclen'] ))  
        else:
            # Seek to next point record
            fh.seek( h['offset'] + ( (i+1) * h['pointreclen'] ))

    f.Destroy()
    ds.Destroy()
    if verbose:
        print "Created new shapefile \n  " + outfile
    fh.close()

def createTxt(outfile, h, numpts=None, rand=True, dim=3, sample=1):

    # Open the LAS file and skip the header
    fh = open(h['infile'],'rb')
    fh.seek(h['offset'])
    fo = open(outfile,'w+')

    for i in range(0,numpts,sample):
        """
        xrecord = struct.unpack('L',fh.read(4))[0]
        yrecord = struct.unpack('L',fh.read(4))[0]
        zrecord = struct.unpack('L',fh.read(4))[0]
        intensity = struct.unpack('H',fh.read(2))[0]
        return_grp = struct.unpack('B',fh.read(1))[0]
        classification = struct.unpack('B',fh.read(1))[0]
        scan_angle = struct.unpack('b',fh.read(1))[0]
        file_marker = struct.unpack('b',fh.read(1))[0]
        user_fld = struct.unpack('H',fh.read(2))[0]
        gps_time = struct.unpack('d',fh.read(8))[0]
        """

        xrecord,yrecord,zrecord,intensity,return_grp,classification, \
        scan_angle, file_marker, user_fld, gps_time = \
        struct.unpack('LLLHBBbbHd',fh.read(28))

        return_num = return_grp & 7
        return_tot = (return_grp >> 3) &7
        scan_dir = (return_grp >> 6) & 1
        scan_edge = (return_grp >> 7)

        if ( ( not (firstOnly or lastOnly) ) or \
             ( return_num == return_tot and lastOnly ) or \
             ( return_num == 1 and firstOnly) ):
            x = (xrecord * h['xscale'] ) + h['xoffset']
            y = (yrecord * h['yscale'] ) + h['yoffset']
            z = (zrecord * h['zscale'] ) + h['zoffset']

        # Create the feature if the values are reasonable
            if h['zmin'] <= z <= h['zmax'] and \
               h['xmin'] <= x <= h['xmax'] and \
               h['ymin'] <= y <= h['ymax']: 

                # print X,Y,Z,label
                s = '%f,%f,%f,%d\n' % (x,y,z,intensity)
                fo.write(s)
            else:
                print "Point excluded. x,y,z is out of range: ", x,y,z

        # Move it along
        if rand:
            # Pick a random point record and seek to it's position
            rp = random.randint(0,h['numptrecords']-1)
            fh.seek( h['offset'] + ( rp * h['pointreclen'] ))  
        else:
            # Seek to next point record
            fh.seek( h['offset'] + ( (i+1) * h['pointreclen'] ))

    fo.close()
    if verbose:
        print "Created XYZ file \n  " + outfile
    fh.close()

def usage ():
    print "pylas.py  version 0.1 "
    print "\npylas comes with ABSOLUTELY NO WARRANTY.  This is free software, and you are free to use and modify it for any purpose.\n"
    print "OGR bindings from the GDAL/OGR library required for Shapefile support.\n"
    print "Usage: "
    print "     pylas.py [OPTIONS] LASFile"
    print " or  pylas.py [OPTIONS]\n"
    print "Options"
    print "-i, --input           LAS format 1.0 or 1.1 file to read from"
    print "-o, --output          filename to write to"
    print "-v, --verbose         increase verbosity"
    print "-n, --numpoints       number of points to read, otherwise all points will be read"
    print "-s, --sample          sample a point from the file every S records"
    print "-r, --random          sample a random set of points, if numpoints is not set, sample 1/2000 of all records"
    print "-z, --3D              produce a 3D shapefile. output in PointZ shapefile format"
    print "-t, --text            produce a XYZ file instead of a Shapefile"
    print "-m, --metadataonly    show metadata for the LAS file"
    print "    --firstonly       only output first returned pulses"
    print "    --lastonly        only output last returned pulses"

    ## field selection not implemented yet
    """
    print "    --with-gps_time   include gps time"
    print "    --with-return_num include rank of this return for pulse"
    print "    --with-return_tot include total number of returns for pulse"
    print "    --with-asprsclass"
    print "                      include ASPRS classification"
    print "    --with-intensity  include pulse intensity returned"
    print "    --with-pulse_angle"
    print "                      include pulse angle rank"
    print "    --with-user_bit   include user field"
    print "    --with-scan_dir   include scan direction"
    print "    --with-scan_edge  include scan edge flag"
    print "    --with-file_marker"
    print "                      include file marker / file id"
    """

    print "-h, --help            show this help screen"
    print"\n"

if __name__=='__main__':

    try:
        opts, args = getopt.getopt(sys.argv[1:], "hi:o:vrn:tms:z", ["help", "output=", "input=", "3d", "3D", "sample=", "random", "numpoints=", "text", "metadataonly", "firstonly", "lastonly"])
    except getopt.GetoptError:
        # print help information and exit:
        usage()
        sys.exit(2)

    verbose = False
    infile = None
    outfile = None
    rand = False
    dim = 2
    pts = None
    wantText = False
    wantMetadata = False
    firstOnly = False
    lastOnly = False
    sample = 1
    for o, a in opts:
        if o == "-v":
            verbose = True
            wantMetadata = True
        if o in ("-h", "--help"):
            usage()
            sys.exit()
        if o in ("-o", "--outfile"):
            outfile = a
        if o in ("-i", "--infile"):
            infile = a
        if o in ("-r", "--random"):
            rand = True
        if o in ("-z", "--3d", "--3D"):
            dim = 3
        if o in ("-n", "--numpoints"):
            pts = int(a)
        if o in ("-t", "--text"):
            wantText = True
        if o in ("-m", "--metadataonly"):
            wantMetadata = True
        if o in ("-s", "--sample"):
            sample = int(a)
        if o in ("--firstonly"):
            firstOnly = True
        if o in ("--lastonly"):
            lastOnly = True

    if infile == None or ( outfile == None and not wantMetadata ):
        usage()
        sys.exit(1)

    header = parseHeader(infile, wantMetadata)

    if rand: 
        import random
        if pts is None:
            # Calculate reasonable number of pts (1/2000th of total?)
            # Eventually base on output resolution of DEM and extents
            pts = header['numptrecords'] / 2000
    if verbose:
            print 'Reading ' + str(pts) + ' random features\n'
    else:
        if pts is None:
            pts = header['numptrecords'] 
    if verbose:
            print 'Reading first ' + str(pts) + ' features\n'

    if sample != None:
        # make sure we scan enough points to produce the desired number of points
        pts = pts * sample

    if pts > header['numptrecords']:
        #make sure we don't scan beyond the files end
        pts = header['numptrecords']

    if outfile == None:
        sys.exit(1)

    if wantText:
        createTxt(outfile, header, pts, rand, dim, sample)
    else:
        # Read all points or a specified number of points?
        try:
            import ogr
        except:
            print 
            print " To use pylas you need the python bindings for OGR"
            print
            sys.exit(1)

            createShp(outfile, header, pts, rand, dim, sample)



