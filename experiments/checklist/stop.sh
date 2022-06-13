#!/bin/bash
kill $(ps aux | grep "rpc_server" | awk '{print $2}')
kill $(ps aux | grep "rpc_server" | awk '{print $2}')
kill $(ps aux | grep "main" | awk '{print $2}')
kill $(ps aux | grep "main" | awk '{print $2}')
