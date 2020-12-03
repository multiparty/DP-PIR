#!/bin/bash

# Display help
if [ -z "$1" ] || [ "$1" == "-h" ]; then
  echo "Usage: ./run_client_online.sh #machine_id #client_id #queries"
  exit 0
fi

# Run parties
valgrind ./bazel-bin/drivacy/client_online --table=drivacy/testdata/table3.json \
    --config=drivacy/testdata/config3.json --machine=$1 --client=$2 --queries=$3
