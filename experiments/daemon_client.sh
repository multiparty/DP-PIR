#!/bin/bash
ORCHASTRATOR="$1"

# Setup directory for storing config and table data.
mkdir -p data

# Notify orchastrator that you exists.
WORKER_ID=$( { curl "$ORCHASTRATOR/giveip/client" 2> /dev/null; } )

echo "I am a client with worker_id ${WORKER_ID}"

# Loop forever..
while true
do
  # Try to sign up to the next computation.
  response=$( { curl "$ORCHASTRATOR/ping/${WORKER_ID}" 2> /dev/null; } )
  if [ "$response" = "WAIT" ]
  then
    sleep 5
    continue
  fi

  # We signed up! read configuration.
  echo "Received job ${response}!"
  params=($response)
  type=${params[0]}
  machine_id=${params[1]}
  client_id=${params[2]}
  pid=""

  # Run the experiment according to $type in the background,
  # store the process ID in pid.
  if [[ $type == "dppir" ]]
  then
    rand=$RANDOM
    # Read configurations.
    curl "$ORCHASTRATOR/config/${WORKER_ID}" > experiments/dppir/config${rand}.json 2> /dev/null
    while [[ $(cat experiments/dppir/config.json) == "WAIT" ]]
    do
      sleep 2
      curl "$ORCHASTRATOR/config/${WORKER_ID}" > experiments/dppir/config${rand}.json 2> /dev/null
      echo "config.."
    done

    ./experiments/dppir/client.sh ${machine_id} ${client_id} \
                                  ${params[3]} ${params[4]} ${params[5]} ${rand}  \
        > client-${machine_id}-${client_id}.log 2>&1 &
    pid=$!

    rm -f experiments/dppir/config${rand}.json
  elif [[ $type == "checklist" ]]
  then
    sleep 5  # wait to ensure that servers have had a chance to boot.
    ./experiments/checklist/run_client.sh ${params[3]} ${params[4]} ${params[5]} \
        > client-${machine_id}-${client_id}.log 2>&1 &
    pid=$!

  elif [[ $type == "sealpir" ]]
  then
    bazel-3.4.1 run //experiments/sealpir:sealpir --config=opt -- \
        --table=${params[3]} --queries=${params[4]} \
        > client-${machine_id}-${client_id}.log 2>&1 &
    pid=$!
  else
    echo "Invalid type ${type}" > client-${machine_id}-${client_id}.log
    sleep 5 &
    pid=$!
  fi

  # Watch out for kill signal sent from orchastrator.
  while [[ -e /proc/$pid ]]
  do
    echo "Job running..."
    if [ "$( { curl "$ORCHASTRATOR/shouldkill/${WORKER_ID}" 2> /dev/null; } )" -eq 1 ]
    then
      echo "Kill"
      kill -9 $pid
      ./experiments/checklist/stop_servers.sh 2>/dev/null
      ./experiments/checklist/stop_servers.sh 2>/dev/null
      echo "Killed by Orchastrator!" >> client-${machine_id}-${client_id}.log
      break
    fi
    sleep 15
  done

  # Done, send log file to orchastrator which includes the time taken.
  echo "Uploading result to orchastrator..."
  curl -H 'Content-Type: text/plain' -X POST \
       --data-binary @client-${machine_id}-${client_id}.log \
       "$ORCHASTRATOR/done/${WORKER_ID}"

  echo "Waiting for new job!"
done
