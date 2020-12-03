import json
from random import randint
import sys


# table_size = sys.argv[1]
# key_size = sys.argv[2]
# value_size = sys.argv[3]

table_size = input("Enter table size: ")
key_size = input("Enter key size in bits: ")
value_size = input("Enter value size in bits: ")

max_key = pow(2,int(key_size))
max_value = pow(2,int(value_size))

print("Max key:", max_key)
print("Max value:", max_value)

table = []
for i in range(int(table_size)):
    pair = {}
    pair["key"] = randint(0, max_key)
    pair["value"] = randint(0, max_value)
    table.append(pair)

with open('table.json', 'w') as fp:
    json.dump(table, fp)
