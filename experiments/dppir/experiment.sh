#!/bin/bash
./scripts/stop_parties.sh

echo "Starting party 2"
./bazel-bin/drivacy/party_online \
  --table=experiments/drivacy/table.json \
  --config=experiments/drivacy/config.json \
  --party=2 --machine=1 \
  --batch=1000000 --span=20 --cutoff=184 \
  > party2.log 2>&1 &

sleep 2

echo "Starting party 1"
./bazel-bin/drivacy/party_online \
  --table=experiments/drivacy/table.json \
  --config=experiments/drivacy/config.json \
  --party=1 --machine=1 \
  --batch=1000000 --span=20 --cutoff=184 \
  > party1.log 2>&1 &

sleep 2

echo "Starting client"
./bazel-bin/drivacy/client_online \
  --table=experiments/drivacy/table.json \
  --config=experiments/drivacy/config.json \
  --machine=1 --client=1 --queries=1000000

./scripts/stop_parties.sh
