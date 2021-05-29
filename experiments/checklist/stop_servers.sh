#!/bin/bash
kill $(ps aux | grep "rpc_server" | awk '{print $2}')
