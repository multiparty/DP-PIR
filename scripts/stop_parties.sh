#!/bin/bash
kill $(ps aux | grep '[\.]/bazel-bin/drivacy/main' | awk '{print $2}')
