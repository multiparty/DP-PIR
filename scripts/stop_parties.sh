#!/bin/bash
kill $(ps aux | grep '[\.]/bazel-bin/drivacy/party_offline' | awk '{print $2}')
kill $(ps aux | grep '[\.]/bazel-bin/drivacy/party_online' | awk '{print $2}')
