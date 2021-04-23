#!/bin/bash

# basically a javascript version of bin2c

echo -n "const $3 = \`data:"
echo -n $1
echo -n ";base64,"
cat $2 | base64 -w0
echo "\`;"
