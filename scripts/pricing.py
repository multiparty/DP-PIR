#!/usr/bin/python3

import sys
import math

HELP_FLAGS = { '-h': True, '--help': True }
PER_UNIT_PRICE = 0.0116

if __name__ == "__main__":
  # Print help message.
  need_help = len(sys.argv) < 2
  for arg in sys.argv:
    if HELP_FLAGS.get(arg, False):
      need_help = True
      break

  if need_help:
    print("Usage: python3 pricing.py <number of parties> <queries per second>")
    sys.exit(0)

  # Read command line arguments.
  parties = int(sys.argv[1])
  persecond = float(sys.argv[2])
  
  secondspermillion = 1000000.0/persecond
  price = (secondspermillion / 3600.0) * PER_UNIT_PRICE * parties
  print(price)
