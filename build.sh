#!/bin/bash
old_dir=$(pwd)
git clone https://github.com/warmcat/libwebsockets.git /tmp/libwebsockets
cd /tmp/libwebsockets
mkdir build
cd build
cmake ..
make
cd $old_dir
export ENV_LDFLAGS="-lssl -lcrypto /tmp/libwebsockets/build/lib/libwebsockets.a" && export STATIC_LIBWEBSOCKETS="true" && export ENV_CFLAGS="-I/tmp/libwebsockets/build/lib/Headers" && make
if [ "$1" = "--install" ]; then
  sudo make install
fi
