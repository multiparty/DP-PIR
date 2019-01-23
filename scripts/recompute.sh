#!/bin/bash

mkdir -p data

if [ ! -f data/server-map.json ]
then
  ./scripts/scrape.sh
fi

curl "http://localhost:9111/recompute/" -m 120 # 2 minutes time out
echo ""

