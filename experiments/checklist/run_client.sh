#!/bin/bash
if [[ $# == 0 ]] || [[ $1 == "-h" ]] || [[ $1 == "--help" ]]; then
  echo "Usage: ./run_client.sh <server1 ip:port> <server2 ip:port> <query count> <pirtype: Punc|DPF>"
  exit 0
fi

echo "Running checklist client with $1 $2 q$3 $4"
cd experiments/checklist
bazel run //:main -c opt -- -rowLen=8 -tls=0 -serverAddr=$1 -serverAddr2=$2 -pirType=$4 q$3
