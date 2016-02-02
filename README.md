# webpipe
A simple utility for using websockets on the command line.

    while true; do date; sleep 1; done | webpipe -f test.html
    
![screenshot](http://i.imgur.com/Muh8VUE.png)

## Installation

Install the latest version of `libwebsockets` [(source here)](https://github.com/warmcat/libwebsockets)

    git clone https://github.com/emgram769/webpipe && cd webpipe
    make
    sudo make install

## Try it out

    make
    ./webpipe -f test.html

Navigate to `localhost:8000` and open your browser's console.
Type what ever you'd like into your terminal.

## Usage

- `-p [port]` to specify a port number to serve on.
- `-f [file]` to host an HTML file (hardcoded mime-type).
- `-d` to see debug messages in stderr.
