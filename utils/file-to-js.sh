#!/bin/bash

echo -n "const $2 = \`data:application/octet-binary;base64,"
cat $1 | base64 -w0
echo "\`;"
