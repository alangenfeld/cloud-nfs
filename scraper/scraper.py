#!/usr/bin/env python

import boto
import os
import tempfile
import pickle
import sys

fname = "/tmp/intercept.log"

FILE = open(fname, "r")

header = FILE.readline()
vals = header.split()
bucketname = "cs699wisc_samanas"

"Get your developer keys from the .boto config file."
config = boto.config


tmpFileName = vals[1]
tmpFile = open(tmpFileName, "w")
data = FILE.read(int(vals[2]))
tmpFile.write(data)
tmpFile.close()


"Glean path from intercept.log"
if len(vals) > 2:
    pathName = vals[3]

"Catch exception of creating new dictionary"
try:
    table = open('table.pkl', 'rb')
# Happens if table.pkl does not exist
except IOError:
    table_create = open('table.pkl', 'wb')
    table_dict = {tmpFileName:pathName}
    pickle.dump(table_dict,table_create)
    table_create.close()
else:
    table_dict = pickle.load(table)
    table.close()
    new_table = open('table.pkl', 'w+b')
    if table_dict.has_key(tmpFileName):
        "Duplicate? Should we not do anything?"
    else:
        table_dict[tmpFileName] = pathName
    pickle.dump(table_dict, new_table)
    new_table.close()


sys.exit()

#tmpFile = open(tmpFileName, "r")

#os.remove(fname)
os.remove(tmpFileName)

sys.exit()

def upload_file (tmpFileName, bucketName):
    "Create source and destination URIs."
    src_uri = boto.storage_uri(tmpFileName, "file")
    dst_uri = boto.storage_uri(bucketName, "gs")

    "Create a new destination URI with the source file name as the object name."
    new_dst_uri = dst_uri.clone_replace_name(tmpFileName)

    "Create a new destination key object."
    dst_key = new_dst_uri.new_key()

    "Retrieve the source key and create a source key object."
    src_key = src_uri.get_key()

    "Create a temporary file to hold your copy operation."
    tmp = tempfile.TemporaryFile()
    src_key.get_file(tmp)
    tmp.seek(0)

    "Upload the file."
    dst_key.set_contents_from_file(tmp)

