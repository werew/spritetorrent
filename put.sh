#!/bin/bash

# This script simply print a PUT msg
# it is useful to perform some tests
# ex of use: ./put.sh | nc -u localhost 5555

TYPE_PUT="\x6e"
TYPE_HASH="\x32"
HASH="aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
TYPE_CLIENT="\x37"
PORT="\xff\xff"
IP="\x00\x00\x00\x00"

printf "$TYPE_PUT\x00\x4c$TYPE_HASH\x00\x40$HASH$TYPE_CLIENT\x00\x06$PORT$IP"
