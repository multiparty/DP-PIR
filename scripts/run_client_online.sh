#!/bin/bash

# Display help
if [ -z "$1" ] || [ "$1" == "-h" ]; then
  echo "Usage: ./run_client_online.sh #machine_id #client_id tablesize #queries"
  exit 0
fi

# Run parties
./bazel-bin/drivacy/client_online --table=$3 \
    --config=drivacy/testdata/config1.json --machine=$1 --client=$2 --queries=$4
