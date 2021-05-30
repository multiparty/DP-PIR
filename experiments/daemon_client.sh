ORCHASTRATOR="$1"

# Setup directory for storing config and table data.
mkdir -p data

# Notify orchastrator that you exists.
curl "$ORCHASTRATOR/giveip/client" 2> /dev/null

# Loop forever..
while true
do
  # Try to sign up to the next computation.
  response=$( { curl "$ORCHASTRATOR/signup/client" 2> /dev/null; } )
  if [ "$response" = "WAIT" ]
  then
    sleep 1
    continue
  fi

  # We signed up! read configuration.
  params=($response)
  type=${params[0]}
  machine_id=${params[1]}
  client_id=${params[2]}
  pid=""

  # Run the experiment according to $type in the background,
  # store the process ID in pid.
  if [[ $type == "dppir" ]]
  then
    # Read table and configurations.
    curl "$ORCHASTRATOR/config" > experiments/dppir/config.json 2> /dev/null
    curl "$ORCHASTRATOR/table" > experiments/dppir/table.json 2> /dev/null
    
    ./experiments/dppir/client.sh ${machine_id} ${client_id}
                                  ${params[3]} ${params[4]} \
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
    if [ "$( { curl "$ORCHASTRATOR/shouldkill" 2> /dev/null; } )" -eq 1 ]
    then
      echo "Kill"
      kill -9 $pid
      echo "Killed by Orchastrator!" >> client-${machine_id}-${client_id}.log
      break
    fi
    sleep 30
  done

  # Done, send log file to orchastrator which includes the time taken.
  curl -H 'Content-Type: text/plain' -X POST \
       --data-binary @client-${machine_id}-${client_id}.log \
       "$ORCHASTRATOR/doneclient/${machine_id}/${client_id}"
done
