#!/bin/bash
killall posix.ganesha.nfsd
make install
posix.ganesha.nfsd -d