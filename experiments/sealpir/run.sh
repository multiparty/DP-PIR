#!/bin/bash
cd experiments/sealpir
bazel run //:sealpir --config=opt -- --table=$1 --queries=$2
