#!/bin/bash
bazel run //DPPIR/online:party -- config/example.txt 0 > party-0.log 2>&1 &
sleep 1

bazel run //DPPIR/online:backend -- config/example.txt > party-1.log 2>&1 &
sleep 1

bazel run //DPPIR/online:client -- config/example.txt $1
