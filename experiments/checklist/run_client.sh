#!/bin/bash
if [[ $# == 0 ]] || [[ $1 == "-h" ]] || [[ $1 == "--help" ]]; then
  echo "Usage: ./run_client.sh <server1 ip:port> <server2 ip:port> <query count>"
  exit 0
fi

echo "Running checklist client with $1 $2 q$3"
bazel-3.4.1 run //experiments/checklist:main -- -rowLen=8 -tls=0 -serverAddr=$1 -serverAddr2=$2 q$3
