# webpipe
A simple command line websocket utility for piping messages to and from a browser

    while true; do date; sleep 1; done | webpipe -f test.html
    
![screenshot](http://i.imgur.com/IlBwCO8.png)

## Installation


    git clone https://github.com/emgram769/webpipe && cd webpipe
    ./build.sh --install
    
Or you can install the latest version of `libwebsockets` [(source here)](https://github.com/warmcat/libwebsockets)
and run `make` instead of `./build.sh`.


## Try it out

    ./build.sh
    ./webpipe -f test.html

Navigate to `localhost:8000` and open your browser's console.
Type what ever you'd like into your terminal.

## Try the client usage

In one shell:

    webpipe
and in another:

    webpipe localhost:8000

## Usage

### Typical
Client:

    webpipe somesite.com
    
Server:

    webpipe -p 5000 -f file.html
    
### Flags
- `-p [port]` to specify a port number to serve on.
- `-f [file]` to host an HTML file (hardcoded mime-type).
- `-i [message]` to send a message once the server is connected.  This is useful for subscription based services.
- `-s` to attempt to connect with SSL.
- `-D [delimiter]` to specify a character other than `\n` as a delimiter.
- `-d` to see debug messages in stderr.
- `-U [max users]` to specify the maximum number of connections.
- `-B [max buffer size]` to specify the maximum size of any sent message.
