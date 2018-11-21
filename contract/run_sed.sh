#!/bin/bash

ORIGIN="$1"
TERMIN="$2"
# echo $ORIGIN
if [ -z "$TERMIN" ];then
    sed \
    -e 's/\"type\": \"name\"/\"type\": \"account_name\"/g' \
    -e 's/\"types\": \[\]/\"types\": \[ { \"new_type_name\": \"account_name\", \"type\": \"name\" } \]/' \
    $ORIGIN 
else
    sed \
    -e 's/\"type\": \"name\"/\"type\": \"account_name\"/g' \
    -e 's/\"types\": \[\]/\"types\": \[ { \"new_type_name\": \"account_name\", \"type\": \"name\" } \]/' \
    $ORIGIN > $TERMIN 
fi
