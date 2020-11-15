#!/bin/bash

# Display help
if [ -z "$1" ] || [ "$1" == "-h" ]; then
  echo "Usage: ./run_client.sh #machine_id #queries"
  exit 0
fi

# Run parties
./bazel-bin/drivacy/client --table=data/server-map.json --config=data/config.json --machine=$1 --queries=$2
