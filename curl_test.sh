#!/bin/bash

for ((i=1; i<=1000; i++))
do
    echo "Making curl call $i"
    if curl -sSf -X GET http:/75.193.159.188/ > /dev/null; then
        echo "Curl call $i was successful"
    else
        echo "Curl call $i failed"
    fi
done
