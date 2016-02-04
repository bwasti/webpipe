#!/bin/bash
old_dir=$(pwd)
git clone https://github.com/warmcat/libwebsockets.git /tmp/libwebsockets
cd /tmp/libwebsockets
mkdir build
cd build
cmake ..
make
cd $old_dir
export ENV_LDFLAGS="-L/tmp/libwebsockets/build/lib" && export ENV_CFLAGS="-I/tmp/libwebsockets/build/lib/Headers" && make
