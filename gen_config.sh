#!/bin/bash
bazel build //drivacy:config
./bazel-bin/drivacy/config --parties=$1 > data/config.json
