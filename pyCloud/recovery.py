#!/usr/bin/env python

import boto
import os
import tempfile
import pickle

#
def read_file(fileName) : 

    return

# 
#bucketName = cloudnfs
bucketName = "cs699wisc_samanas"

def download(srcName, dstName) :
    "Download the files."
    src_uri = boto.storage_uri(bucketName + "/" + srcName, "gs")
    dst_uri = boto.storage_uri(dstName, "file")
    
    "Append the object name to the directory name."
    dst_key_name = dst_uri.object_name# + os.sep + src_uri.object_name
    
    "Use the new destination key name to create a new destination URI."
    new_dst_uri = dst_uri.clone_replace_name(dst_key_name)
    print new_dst_uri
    
    "Create a new destination key object."
    dst_key = new_dst_uri.new_key()
    
    "Retrieve the source key and create a source key object."
    src_key = src_uri.get_key()
    
    "Create a temporary file to hold our copy operation."
    tmp = tempfile.TemporaryFile()
    src_key.get_file(tmp)
    tmp.seek(0)

    "Download the object."
    dst_key.set_contents_from_file(tmp)
    return

#########################################################################
# Recovery
#########################################################################

# "Load your developer keys from the .boto config file."
config = boto.config

#"Create a URI, but don't specify a bucket or object because you are listing buckets."
uri = boto.storage_uri("", "gs")

#"Get your buckets."
buckets = uri.get_all_buckets()
l = list();
for bucket in buckets:
    
    "Create a URI for a bucket."
    uri = boto.storage_uri(bucket.name, "gs")

    "Get the objects that are in the bucket."
    objs = uri.get_bucket()
    for obj in objs :
        if (obj.name == 'table.pkl') :
            download(obj.name, obj.name)
        else :
            download(obj.name, "/" + obj.name)

#if 'table.pkl' in l :
#    download('table.pkl', 'temp_table.pkl')

#    table = open(temp_table.pkl)
#    table_dict = pickle.load(table)

