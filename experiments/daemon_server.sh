#!/bin/bash
ORCHASTRATOR="$1"
rm -f experiments/dppir/config*.json

# Setup directory for storing config and table data.
mkdir -p data

# Notify orchastrator that you exists.
WORKER_ID=$( { curl "$ORCHASTRATOR/giveip/server" 2> /dev/null; } )

echo "I am a server with worker_id ${WORKER_ID}"

# Loop forever..
while true
do
  echo "Waiting for new job!"

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
  outfile=""
  pid=""

  # Run the experiment according to $type in the background,
  # store the process ID in pid.
  if [[ $type == "dppir" ]]
  then
    # Read rest of parameters.
    party_id=${params[1]}
    server_id=${params[2]}
    online=${params[3]}

    # Read configurations.
    echo "Reading config ..."
    config="config/party-${party_id}-${server_id}.txt"
    curl "$ORCHASTRATOR/config/${WORKER_ID}" > $config 2> /dev/null
    while [[ $(cat ${config} 2> /dev/null) == "WAIT" ]]
    do
      sleep 2
      echo "retry config..."
      curl "$ORCHASTRATOR/config/${WORKER_ID}" > $config 2> /dev/null
    done

    # Run binary.
    outfile="DPPIR-party-${party_id}-${server_id}.log"
    bazel run --config=opt //DPPIR:main -- \
        --config=$config --role=party --stage=$online \
        --party_id=$party_id --server_id=$server_id > $outfile 2>&1 &
    pid=$!
  elif [[ $type == "checklist" ]]
  then
    # Read rest of parameters.
    server=${params[1]}
    table_size=${params[2]}
    port=${params[3]}
    pirtype=${params[4]}
    outfile="checklist-server-${server}.log"
    # Run binary.
    ./experiments/checklist/run_server.sh $table_size $port $pirtype > $outfile 2>&1 &
    pid=$!
  else
    outfile="invalid-error.log"
    echo "Invalid type ${type}" > $outfile
    sleep 5 &
    pid=$!
  fi

  # Watch out for kill signal sent from orchastrator.
  while [[ -e /proc/$pid ]]
  do
    echo "Job running..."
    action=$( { curl "$ORCHASTRATOR/shouldkill/${WORKER_ID}" 2> /dev/null; } )
    if [[ "$action" == "2" ]]
    then
      echo "Log requested..."
      curl -H 'Content-Type: text/plain' -X POST --data-binary @${outfile} "$ORCHASTRATOR/showlog/${WORKER_ID}"
    elif [[ "$action" == "1" ]]
    then
      echo "Kill"
      kill -9 $pid
      kill $(ps aux | grep "DPPIR/main" | awk '{print $2}') 2>/dev/null
      kill $(ps aux | grep "DPPIR/main" | awk '{print $2}') 2>/dev/null
      ./experiments/checklist/stop.sh 2>/dev/null
      echo "Killed by Orchastrator!" >> $outfile
      break
    fi
    sleep 15
  done

  # Done, send log file to orchastrator which includes the time taken.
  echo "Uploading result to orchastrator..."
  curl -H 'Content-Type: text/plain' -X POST --data-binary @${outfile} "$ORCHASTRATOR/done/${WORKER_ID}"
done
