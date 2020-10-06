#!/bin/bash

# Display help
if [ -z "$1" ] || [ "$1" == "-h" ]; then
  echo "Usage: ./run_parties.sh #parties #batch_size"
  exit 0
fi

# Run parties
for (( party=$1; party>0; party-- ))
do
  echo "Running party $party"
  ./bazel-bin/drivacy/main --table=data/server-map.json --config=data/config.json --party=$party --batch=$2 > party$party.log 2>&1 &
  sleep 1
done
