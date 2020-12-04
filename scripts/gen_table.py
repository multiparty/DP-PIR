#!/usr/bin/python3
import json
from random import randint
import sys

HELP_FLAGS = { '-h': True, '--help': True }

if __name__ == "__main__":
  # Print help message.
  need_help = len(sys.argv) < 2
  for arg in sys.argv:
    if HELP_FLAGS.get(arg, False):
      need_help = True
      break

  if need_help:
    print("Usage: python3 gen_table.py <table_size>")
    print("Always outputs to table.json")
    sys.exit(0)

table_size = int(sys.argv[1])
max_value = pow(10,6)
table = {'table': []}
for i in range(int(table_size)):
    pair = {}
    pair["key"] = i
    pair["value"] = randint(0, max_value)
    table['table'].append(pair)
    
with open('table.json', 'w') as fp:
    json.dump(table, fp)
