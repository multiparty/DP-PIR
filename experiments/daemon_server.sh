#!/bin/bash
ID=$RANDOM
ORCHASTRATOR="$1"

# Setup directory for storing config and table data.
mkdir -p data

# Notify orchastrator that you exists.
WORKER_ID=$( { curl "$ORCHASTRATOR/giveip/server" 2> /dev/null; } )

echo "I am a server with worker_id ${WORKER_ID}"

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
  party_id=${params[1]}
  machine_id=${params[2]}
  pid=""

  # Run the experiment according to $type in the background,
  # store the process ID in pid.
  if [[ $type == "dppir" ]]
  then
    # Read configurations.
    curl "$ORCHASTRATOR/config/${WORKER_ID}" > experiments/dppir/config${ID}.json 2> /dev/null
    while [[ $(cat experiments/dppir/config.json) == "WAIT" ]]
    do
      sleep 2
      curl "$ORCHASTRATOR/config/${WORKER_ID}" > experiments/dppir/config${ID}.json 2> /dev/null
      echo "config.."
    done

    ./experiments/dppir/server.sh ${party_id} ${machine_id} ${params[3]} \
                                  ${params[4]} ${params[5]} ${params[6]} ${params[7]} ${ID} \
        > party-${party_id}-${machine_id}.log 2>&1 &
    pid=$!
  elif [[ $type == "checklist" ]]
  then
    ./experiments/checklist/run_server.sh ${params[3]} ${params[4]} \
        > party-${party_id}-${machine_id}.log 2>&1 &
    pid=$!
  else
    echo "Invalid type ${type}" > party-${party_id}-${machine_id}.log
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
      echo "Killed by Orchastrator!" >> party-${party_id}-${machine_id}.log
      break
    fi
    sleep 15
  done

  # Done, send log file to orchastrator which includes the time taken.
  echo "Uploading result to orchastrator..."
  curl -H 'Content-Type: text/plain' -X POST \
       --data-binary @party-${party_id}-${machine_id}.log \
       "$ORCHASTRATOR/done/${WORKER_ID}"

    echo "Waiting for new job!"
done
