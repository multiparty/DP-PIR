#!/usr/bin/python3
import json
from random import randint
import sys


# table_size = sys.argv[1]
# key_size = sys.argv[2]
# value_size = sys.argv[3]

table_size = input("Enter table size: ")

max_value = pow(10,8)

print("Max value:", max_value)

table = []
for i in range(int(table_size)):
    pair = {}
    pair["key"] = i
    pair["value"] = randint(0, max_value)
    table.append(pair)

with open('table.json', 'w') as fp:
    json.dump(table, fp)
