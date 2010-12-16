#!/usr/bin/env python

import boto
import os
from stat import *
import tempfile
import sys
import cloudnfs

def walktree(top, meta_list):
    for f in os.listdir(top):
        pathname = os.path.join(top, f)
        mode = os.stat(pathname)[ST_MODE]
        if S_ISDIR(mode):
            walktree(pathname, meta_list)
        elif S_ISREG(mode):
            cloudnfs.send_file(pathname, pathname)
            cloudnfs.create_entry(pathname, meta_list)
        # We don't cover symbolic links or stuff like that yet...
        else:
            print 'Skipping %s' % pathname
    #cloudnfs.print_meta()

###############################################################################

if len(sys.argv) != 2:
    print "Please supply the mount point of the nfs volume you'd like to back\
up."
    sys.exit()

mnt_point = sys.argv[1]

# Change to given path in argv[1]..if it is indeed a path
os.chdir(mnt_point)

meta_list = cloudnfs.get_list()
#print meta_list
walktree(mnt_point, meta_list)
#for m in meta_list:
#    m.print_e()
cloudnfs.store_list(meta_list)
