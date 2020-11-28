ORCHASTRATOR="$1"

# Setup directory for storing config and table data.
mkdir -p data

# Notify orchastrator that you exists.
curl "$ORCHASTRATOR/giveip/party" 2> /dev/null

# Loop forever..
while true
do
  # Try to sign up to the next computation.
  response=$( { curl "$ORCHASTRATOR/signup/party" 2> /dev/null; } )
  if [ "$response" = "WAIT" ]
  then
    sleep 1
    continue
  fi

  # We signed up! read configuration.
  params=($response)
  party_id=${params[0]}
  machine_id=${params[1]}
  batches=${params[2]}
  batch=${params[3]}
  span=${params[4]}
  cutoff=${params[5]}

  # Read table and configurations.
  curl "$ORCHASTRATOR/config" > data/config.json 2> /dev/null
  curl "$ORCHASTRATOR/table" > data/table.json 2> /dev/null

  # Run client and time the command
  echo "Running party for ${party_id}-${machine_id} with ${batch} ${span} ${cutoff}"
  ./bazel-bin/drivacy/main --config=data/config.json --table=data/table.json \
                           --party=${party_id} --machine=${machine_id} \
                           --batches=${batches} --batch=${batch} --span=${span} \
                           --cutoff=${cutoff} > party-${party_id}-${machine_id}.log &
  pid=$!

  # Watch out for kill signal sent from orchastrator.
  while true
  do
    if [ "$( { curl "$ORCHASTRATOR/shouldkill" 2> /dev/null; } )" -eq 1 ]
    then
      echo "Kill!"
      kill -9 $pid
      break
    fi
    sleep 30
  done
done
