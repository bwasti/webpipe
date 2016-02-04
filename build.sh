#!/bin/bash
old_dir=$(pwd)
git clone https://github.com/warmcat/libwebsockets.git /tmp/libwebsockets
cd /tmp/libwebsockets
mkdir build
cd build
cmake ..
make
cd $old_dir
export EXTRA_LDFLAGS="-L/tmp/libwebsockets/build/lib" && make
