#!/bin/bash

PORT=9110
PARTIES="$1"
REPLICAS="$2"

if [ -z $PARTIES ]; then
    PARTIES=2
fi
if [ -z $REPLICAS ]; then
    REPLICAS=2
fi

if [ "$(basename $(pwd))" == "scripts" ]
then
    cd ..
fi

# Write out config file
echo "{" > ./parties/config.json
echo "\"parties\": $PARTIES," >> ./parties/config.json
echo "\"replicas\": $REPLICAS," >> ./parties/config.json
echo "\"base_port\": $PORT" >> ./parties/config.json
echo "}" >> ./parties/config.json

# run server
mkdir -p logs
node server.js > logs/server.log &

# run all back-ends and front-ends
owner=1
while [ $owner -le $PARTIES ]; do
    replica=1
    while [ $replica -le $REPLICAS ]; do
        if [ $owner == 1 ]; then
            node parties/backend.js "${owner}" "${replica}" & #> ./logs/backend-${replica}.log &
        else
            node parties/frontend.js "${owner}" "${replica}" & #> ./logs/frontend-${owner}-${replica}.log &
        fi

        replica=$((${replica}+1))
    done
    owner=$((${owner}+1))
done

