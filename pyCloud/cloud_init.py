#!/usr/bin/env python

import boto
import os
from stat import *
import tempfile
import sys
import glob

bucketName = "cs699wisc_samanas"
#bucketName = "cloudnfs"

def send_file(tmpFileName) :
#   "Create source and destination URIs."
    src_uri = boto.storage_uri(tmpFileName, "file")
    dst_uri = boto.storage_uri(bucketName, "gs")
    
#    "Create a new destination URI with the source file name as the object name."
    new_dst_uri = dst_uri.clone_replace_name(tmpFileName)
    
#    "Create a new destination key object."
    dst_key = new_dst_uri.new_key()
    
#    "Retrieve the source key and create a source key object."
    src_key = src_uri.get_key()
    
#    "Create a temporary file to hold your copy operation."
    tmp = tempfile.TemporaryFile()
    src_key.get_file(tmp)
    tmp.seek(0)
    
#    "Upload the file."
    dst_key.set_contents_from_file(tmp)
    return

def walktree(top):
    for f in os.listdir(top):
        pathname = os.path.join(top, f)
        mode = os.stat(pathname)[ST_MODE]
        if S_ISDIR(mode):
            walktree(pathname)
        elif S_ISREG(mode):
            send_file(pathname)
        # We don't cover symbolic links or stuff like that yet...
        else:
            print 'Skipping %s' % pathname

###############################################################################

if len(sys.argv) != 2:
    print "Please supply the mount point of the nfs volume you'd like to back\
up."
    sys.exit()

mnt_point = sys.argv[1]

# Change to given path in argv[1]..if it is indeed a path
os.chdir(mnt_point)

walktree(mnt_point)
