#!/usr/bin/env python

import os
import boto
import tempfile
import pickle
import re
import cloudnfs

#########################################################################
fname = "/tmp/intercept.log"

FILE = open(fname, "r")
#build_rm_list(FILE)

#   "Get your developer keys from the .boto config file."
config = boto.config

while 1: ################################################################
    header = FILE.readline()
    if (len(header) < 4) :
        break
    vals = header.split()
    # v0 - num, v1 - size, v2 - path

    tmpFile = tempfile.NamedTemporaryFile()
    if re.search('remove', vals[0]):
        print "Removed " + vals[2] + "\n"
        continue
    data = FILE.read(int(vals[1]))
    tmpFile.write(data)
    dstName = vals[2]

    "Catch exception of creating new dictionary"
    try:
        table = open('table.pkl', 'rb')
    # Happens if table.pkl does not exist
    except IOError:
        table_create = open('table.pkl', 'wb')
        table_dict = {vals[0]:vals[2]}
        pickle.dump(table_dict,table_create)
        table_create.close()
    else:
        table_dict = pickle.load(table)
        table.close()
        new_table = open('table.pkl', 'w+b')
        if table_dict.has_key(vals[0]):
            "Duplicate? Should we not do anything?"
        else:
            table_dict[vals[0]] = vals[2]
        pickle.dump(table_dict, new_table)
        new_table.close()
    
    # send the write to GS
    cloudnfs.send_file(tmpFile.name, dstName);
################ WHILE 1 ######################################

cloudnfs.send_file('table.pkl', 'table.pkl')

os.remove(fname)

