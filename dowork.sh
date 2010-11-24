#!/bin/bash
killall posix.ganesha.nfsd
make install | grep warning
posix.ganesha.nfsd -d
