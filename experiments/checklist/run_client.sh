#!/bin/bash
if [[ $# == 0 ]] || [[ $1 == "-h" ]] || [[ $1 == "--help" ]]; then
  echo "Usage: ./run_client.sh <table size> <server1 ip:port> <server2 ip:port> <query count>"
  exit 0
fi

bazel-3.4.1 run //experiments/checklist:main --config=opt -- -numRows=$1 -rowLen=8 -tls=0 -serverAddr=$2 -serverAddr2=$3 q$4
