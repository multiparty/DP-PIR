#!/bin/bash
echo "Make sure you first run orchastrator with:"
echo "$ cd <DPPIR>/experiments/orchestrator"
echo "$ npm install"
echo "$ node main"
echo ""

# Compile code
echo "Building code in opt mode"
bazel build --config=opt //DPPIR:main
bazel build --config=opt //DPPIR/config:gen_config

# Command line args
servers=$1
clients=$2
if [[ $servers == "" || $clients == "" ]]; then
  echo "Usage: ./experiments/local.sh <#servers> <#clients>"
  exit 0
fi

# trap ctrl-c and call ctrl_c()
trap ctrl_c INT
function ctrl_c() {
  echo "Killing all workers"
  # Kill all servers and clients.
  kill $(ps aux | grep "experiments/daemon_server.sh" | awk '{print $2}') 2>/dev/null
  kill $(ps aux | grep "experiments/daemon_server.sh" | awk '{print $2}') 2>/dev/null
  kill $(ps aux | grep "experiments/daemon_client.sh" | awk '{print $2}') 2>/dev/null
  kill $(ps aux | grep "experiments/daemon_client.sh" | awk '{print $2}') 2>/dev/null  
  # Kill any running experiments.
  kill $(ps aux | grep "DPPIR/main" | awk '{print $2}') 2>/dev/null
  kill $(ps aux | grep "DPPIR/main" | awk '{print $2}') 2>/dev/null
  ./experiments/checklist/stop.sh 2>/dev/null
  ./experiments/sealpir/stop.sh 2>/dev/null
}

# Run servers and clients
pids=()
for i in $(seq 1 $servers); do
  ./experiments/daemon_server.sh 127.0.0.1:8000 > experiments/server${i}.log 2>&1 &
  pids=( "${pids[@]}" "$!" )
done
for i in $(seq 1 $clients); do
  ./experiments/daemon_client.sh 127.0.0.1:8000 > experiments/client${i}.log 2>&1 &
  pids=( "${pids[@]}" "$!" )
done

# Do not terminate until daemons exit
for pid in ${pids[@]}; do
  wait $pid
done
