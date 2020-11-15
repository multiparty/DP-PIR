#!/bin/bash

# Display help
if [ -z "$1" ] || [ "$1" == "-h" ]; then
  echo "Usage: ./run_parties.sh #parties #parallelism #batch_size"
  exit 0
fi

# Run parties
for (( party=$1; party>0; party-- ))
do
  for (( machine=1; machine<=$2; machine++ ))
  do
    echo "Running party $party-$machine"
    ./bazel-bin/drivacy/main --table=data/server-map.json --config=data/config.json --party=$party --machine=$machine --batch=$3 > party$party-$machine.log 2>&1 &
    sleep 1
  done
done
