import boto
import os
import tempfile

bucketName = "cloudnfs"
#bucketName = "cs699wisc_samanas"

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

def send_file(srcName, dstName) :
#   "Create source and destination URIs."
    src_uri = boto.storage_uri(srcName, "file")
    dst_uri = boto.storage_uri(bucketName, "gs")
    
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

class entry :
    def __init__(name):
        self.name = name
        self.versions = 1
        self.version_map = dict({1:get_curr_bucket()})
    def update():
        self.versions += 1
        self.version_map[self.versions] = get_curr_bucket()
    

def create_entry(name):
    entry_list.append(entry(name))






