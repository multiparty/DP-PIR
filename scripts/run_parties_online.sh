#!/bin/bash

# Display help
if [ -z "$1" ] || [ "$1" == "-h" ]; then
  echo "Usage: ./run_parties_online.sh #parties #parallelism tablesize batch_size span cutoff"
  exit 0
fi

# Run parties
for (( party=$1; party>0; party-- ))
do
  for (( machine=1; machine<=$2; machine++ ))
  do
    echo "Running party $party-$machine"
    ./bazel-bin/drivacy/party_online --table=$3 \
        --config=drivacy/testdata/config1.json --party=$party \
        --machine=$machine --batch=$4 --span=$5 \
        --cutoff=$6 > party$party-$machine.log 2>&1 &
    sleep 1
  done
done
