#!/bin/bash
if [ -z "$1" ] || [ "$1" == "-h" ]; then
  echo "Usage: gen_config #parties #parallelism [optiona: ip1 ip2 ip3 ...] > path/to/config.json"
  exit 0
fi

# Generate the config and dump it on stdout
./bazel-bin/drivacy/config --parties=$1 --parallelism=$2 ${@:3:$#}
