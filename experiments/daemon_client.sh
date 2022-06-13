#!/bin/bash
ORCHASTRATOR="$1"
rm -f experiments/dppir/config*.json

# Setup directory for storing config and table data.
mkdir -p data

# Notify orchastrator that you exists.
WORKER_ID=$( { curl "$ORCHASTRATOR/giveip/client" 2> /dev/null; } )

echo "I am a client with worker_id ${WORKER_ID}"

# Set the socket buffer size
sudo sysctl -w net.core.rmem_max=123289600
sudo sysctl -w net.core.wmem_max=123289600

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
    server_id=${params[1]}
    queries=${params[2]}
    online=${params[3]}

    # Read configurations.
    echo "Reading config ..."
    config="config/client-${server_id}.txt"
    curl "$ORCHASTRATOR/config/${WORKER_ID}" > $config 2> /dev/null
    while [[ $(cat ${config} 2> /dev/null) == "WAIT" ]]
    do
      sleep 2
      echo "retry config..."
      curl "$ORCHASTRATOR/config/${WORKER_ID}" > $config 2> /dev/null
    done

    # Run binaries.
    outfile="DPPIR-client-${server_id}.log"
    echo "Running client for ${server_id}"
    bazel run --config=opt //DPPIR:main -- \
        --config=$config --role=client --stage=$online \
        --server_id=$server_id --queries=$queries > $outfile 2>&1 &
    pid=$!
  elif [[ $type == "checklist" ]]
  then
    # Read rest of parameters.
    server1=${params[1]}
    server2=${params[2]}
    queries=${params[3]}
    pirtype=${params[4]}
    outfile="checklist-client.log"

    sleep 15  # wait to ensure that servers have had a chance to boot.
    ./experiments/checklist/run_client.sh $server1 $server2 $queries $pirtype > $outfile 2>&1 &
    pid=$!

  elif [[ $type == "sealpir" ]]
  then
    # Read rest of parameters.
    table_size=${params[1]}
    queries=${params[2]}
    outfile="sealpir.log"

    ./experiments/sealpir/run.sh $table_size $queries > $outfile 2>&1 &
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
      ./experiments/sealpir/stop.sh 2>/dev/null
      echo "Killed by Orchastrator!" >> $outfile
      break
    fi
    sleep 15
  done

  # Done, send log file to orchastrator which includes the time taken.
  echo "Uploading result to orchastrator..."
  curl -H 'Content-Type: text/plain' -X POST --data-binary @$outfile "$ORCHASTRATOR/done/${WORKER_ID}"
done
