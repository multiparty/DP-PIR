#!/bin/bash
if [[ $# == 0 ]] || [[ $1 == "-h" ]] || [[ $1 == "--help" ]]; then
  echo "Usage: ./run_server.sh <table size> <port> <pirtype: Punc|DPF>"
  exit 0
fi

echo "Running checklist server on port $2 for $1 rows with pirtype=$3"
cd experiments/checklist
bazel run @checklist//cmd/rpc_server -c opt -- -numRows=$1 -rowLen=8 -tls=0 -p=$2 -pirType=$3
