#!/bin/bash

# Create an empty output directory
rm -rf output
mkdir output

# activate python virtual env
if [ ! -d env ]; then
  echo "Installing dependencies..."
  ./dependencies.sh
fi

# dependencies are installed!
. env/bin/activate

# Run scrape script
echo "Scraping..."
python get_data.py
deactivate
echo ""
echo ""

# Hash and move outputs
echo "Hashing..."
node hash.js
echo ""
echo ""

mkdir -p ../data
mv output/client-ready-data.js ../data/client-map.js
mv output/server-ready-data.json ../data/server-map.json

# cleanup
echo "Clean up..."
rm -rf output



