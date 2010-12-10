#!/usr/bin/env python

import boto
import os
from stat import *
import tempfile
import sys
import glob
import cloudnfs

def walktree(top):
    for f in os.listdir(top):
        pathname = os.path.join(top, f)
        mode = os.stat(pathname)[ST_MODE]
        if S_ISDIR(mode):
            walktree(pathname)
        elif S_ISREG(mode):
            cloudnfs.create_entry(pathname)
            cloudnfs.send_file(pathname)
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
