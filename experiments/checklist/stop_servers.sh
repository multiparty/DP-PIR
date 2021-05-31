#!/bin/bash
kill $(ps aux | grep "rpc_server" | awk '{print $2}')
kill $(ps aux | grep "experiments/checklist" | awk '{print $2}')
