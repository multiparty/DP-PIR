#!/bin/bash
CONFIG=experiments/dppir/config.json

# Read args
party_id=$1
machine_id=$2
batch=$3
span=$4
cutoff=$5
online="$6"
table=$7

echo "Running ${online} party for ${party_id}-${machine_id} with ${batch} ${span} ${cutoff} and table ${table}"
./bazel-bin/drivacy/party_${online} \
    --config=${CONFIG} --table=${table} --party=${party_id} \
    --machine=${machine_id} --batch=${batch} --span=${span} --cutoff=${cutoff}
