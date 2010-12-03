#!/usr/bin/env python

import boto
import os
import tempfile
import pickle

def send_file(srcName, dstName) :
#   "Create source and destination URIs."
    src_uri = boto.storage_uri(srcName, "file")
    dst_uri = boto.storage_uri("cloudnfs", "gs")
    
#    "Create a new destination URI with the source file name as the object name."
    new_dst_uri = dst_uri.clone_replace_name(dstName)
    
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
    send_file(tmpFile.name, dstName);
################ WHILE 1 ######################################

send_file('table.pkl', 'table.pkl')

os.remove(fname)

