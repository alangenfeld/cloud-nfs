#!/usr/bin/env python

import boto
import os
import tempfile
import pickle
import ./cloudnfs.py

#bucketName = cloudnfs
bucketName = "cs699wisc_samanas"


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
            cloudnfs.download(obj.name, obj.name)
        else :
            cloudnfs.download(obj.name, "/" + obj.name)

#if 'table.pkl' in l :
#    download('table.pkl', 'temp_table.pkl')

#    table = open(temp_table.pkl)
#    table_dict = pickle.load(table)

