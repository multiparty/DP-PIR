#!/bin/bash
if [ -z "$1" ] || [ "$1" == "-h" ]; then
  echo "Usage: gen_config #parties > path/to/config.json"
fi

# Generate the config and dump it on stdout
./bazel-bin/drivacy/config --parties=$1 
