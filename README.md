# webpipe
A simple command line websocket utility for piping messages to and from a browser

### Server

    while true; do date; sleep 1; done | webpipe -f test.html

### Client
    
    webpipe -i '{"type":"subscribe","product_id":"BTC-USD"}' wss://ws-feed.exchange.coinbase.com|sed -n 's/.*price":"\([0-9\.]*\)".*/\1/p'


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

## Use it as a simple chat

In one shell:

    webpipe
and in another:

    webpipe localhost:8000

## Usage

####Client:

    webpipe somesite.com
    
for example

    webpipe -i '{"type":"subscribe","product_id":"BTC-USD"}' wss://ws-feed.exchange.coinbase.com

    
####Server:

    webpipe -p 5000 -f file.html
    
#### Flags
- `-p [port]` to specify a port number to serve on.
- `-f [file]` to host an HTML file (hardcoded mime-type).
- `-i [message]` to send a message once the server is connected.  This is useful for subscription based services.
- `-s` to attempt to connect with SSL.
- `-d` to see debug messages in stderr.
- `-U [max users]` to specify the maximum number of connections.
- `-B [max buffer size]` to specify the maximum size of any sent message.
