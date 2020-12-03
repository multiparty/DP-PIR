ORCHASTRATOR="http://3.21.228.215:8000"

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
  machine_id=${params[0]}
  client_id=${params[1]}
  queries=${params[2]}
  online=${params[3]}

  table_arg="--table=data/table.json"
  if [[ $online == "offline" ]]
  then
    table_arg=""
  fi

  # Read table and configurations.
  curl "$ORCHASTRATOR/config" > data/config.json 2> /dev/null
  curl "$ORCHASTRATOR/table" > data/table.json 2> /dev/null

  # Run client and time the command
  echo "Running client for ${machine_id} with ${queries}"
  \time -f "%e" ./bazel-bin/drivacy/client_${online} --config=data/config.json \
                ${table_arg} --machine=${machine_id} --queries=${queries} \
                1> client-${machine_id}-${client_id}.log 2> time.log && \
      curl "$ORCHASTRATOR/doneclient/${machine_id}/${client_id}/$(tail -1 time.log)" \
          2> /dev/null &
  pid=$!

  # Watch out for kill signal sent from orchastrator.
  while true
  do
    if [ "$( { curl "$ORCHASTRATOR/shouldkill" 2> /dev/null; } )" -eq "1" ]
    then
      echo "Kill!"
      kill -9 $pid
      break
    fi
    sleep 30
  done
done
