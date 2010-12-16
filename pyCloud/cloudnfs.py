import boto
import os
import tempfile
import pickle

#bucketName = "cloudnfs"
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
    def __init__(self, name):
        self.name = name
        self.versions = 1
        self.version_map = dict({1:get_curr_bucket()})
        self.active = True
    # This assumes that different versions will always be in a different bucket
    # Thus, versioning not supported yet.
    def update():
        self.versions += 1
        self.version_map[self.versions] = get_curr_bucket()
    def __str__(self):
        return self.name
    def print_e(self):
        print '%s; Version: %d' % (self.name, self.versions)
        print self.active
        print self.version_map

def get_curr_bucket():
    return bucketName

def print_meta():
    meta_list = get_list()
    for e in meta_list :
        print e

def create_entry(name, entry_list):
    entry_list.append(entry(name))
    

# Creates list if doesn't already exist. If it does, save it.
def store_list(new_list):
    meta_list = open('meta.pkl', 'wb')
    pickle.dump(new_list, meta_list)
    meta_list.close()
    send_file('meta.pkl', 'meta.pkl')

def get_list():
    try:
        # meta.pkl shall be stored in the cloud, we will attempt to download
        download('meta.pkl', 'meta.pkl')
    except boto.exception.InvalidUriError:
        # If it isn't there, create locally and instantiate new list.
        meta_list = open('meta.pkl', 'wb')
        new_list = []
    else:
        # meta.pkl exists, lets load it and return.
        meta_list = open('meta.pkl', 'rwb')
        try:
            new_list = pickle.load(meta_list)
        except EOFError:
            # The list we downloaded is empty. Return empty list.
            new_list = []
    meta_list.close()
    return new_list
