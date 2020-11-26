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
  machine_id=${params[0]}
  client_id=${params[1]}
  queries=${params[2]}

  # Read table and configurations.
  curl "$ORCHASTRATOR/config" > data/config.json 2> /dev/null
  curl "$ORCHASTRATOR/table" > data/table.json 2> /dev/null

  # Run client and time the command
  echo "Running client for ${machine_id} with ${queries}"
  \time -f "%e" ./bazel-bin/drivacy/client --config=data/config.json --table=data/table.json --machine=${machine_id} --queries=${queries} 1> client-${machine_id}-${client_id}.log 2> time.log && curl "$ORCHASTRATOR/done/${machine_id}/${client_id}/$(cat time.log)" 2> /dev/null &
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
