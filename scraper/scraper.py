#!/usr/bin/env python

import boto
import os
import tempfile

fname = "/tmp/intercept.log"

FILE = open(fname, "r")

header = FILE.readline()
vals = header.split()
bucketname = "cloudnfs"

"Get your developer keys from the .boto config file."
config = boto.config


tmpFileName = vals[0] 
tmpFile = open(tmpFileName, "w")
data = FILE.read(int(vals[1]))
tmpFile.write(data)
tmpFile.close()
tmpFile = open(tmpFileName, "r")

"Create source and destination URIs."
src_uri = boto.storage_uri(tmpFileName, "file")
dst_uri = boto.storage_uri(bucketname, "gs")

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

os.remove(fname)
os.remove(tmpFileName)
