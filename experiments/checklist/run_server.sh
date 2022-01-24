#!/bin/bash
if [[ $# == 0 ]] || [[ $1 == "-h" ]] || [[ $1 == "--help" ]]; then
  echo "Usage: ./run_server.sh <table size> <port>"
  exit 0
fi

echo "Running checklist server on port $2 for $1 rows"
bazel-3.4.1 run @checklist//cmd/rpc_server -- -numRows=$1 -rowLen=8 -tls=0 -p=$2
