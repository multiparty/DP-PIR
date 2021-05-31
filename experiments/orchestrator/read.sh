#!/bin/bash

for d in $(ls outputs | grep $1)
do
  echo $d
  echo ""
  cat outputs/$d/$2*.log
  echo "=============================="
  echo "=============================="
  echo "=============================="
  echo ""
  echo ""
done
