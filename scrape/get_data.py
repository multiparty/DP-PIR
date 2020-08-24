# Python 3
import json
import geojson
# if this does not work [specifically on Linux]:
# 1. Open the geoql __init__.py file inside the installed package with an editor
# 2. replace all "from geoql.geoql import <whatever>" with "from geoql import <whatever>"
from geoql import geoql
import requests
from queue import Queue

############################ CONFIGURATIONS
RADIUS = 5
RAW_OUTPUT_PATH = './output/raw.geojson';
CLIENT_OUTPUT_PATH = './output/client-raw-data.json';
SERVER_OUTPUT_PATH = './output/server-raw-data.json';


############################ SCRAPE MAP

url = 'https://raw.githubusercontent.com/Data-Mechanics/geoql/master/examples/'

# Boston ZIP Codes regions.
z = geoql.loads(requests.get(url + 'example_zips.geojson').text, encoding="latin-1")

# Extract of street data.
g = geoql.loads(requests.get(url + 'example_extract.geojson').text, encoding="latin-1")


############################ FILTER MAP

g = g.properties_null_remove()\
     .tags_parse_str_to_dict()\
     .keep_by_property({"highway": {"$in": ["residential", "secondary", "tertiary"]}})
g = g.keep_within_radius((42.344936, -71.086976), RADIUS, 'miles') # 4 miles from Boston Common.

g = g.keep_that_intersect(z) # Only those entries found in a Boston ZIP Code regions.
g = g.node_edge_graph() # Converted into a graph with nodes and edges.
g.dump(open(RAW_OUTPUT_PATH, 'w'))


########################### FORMAT MAP AS A GRAPH

points = []
shapes = []
for k in g["features"]:
  if k.type == 'Point':
    points.append(k)
  else:
    shapes.append(k)

# make points in the format of client
nodes = []
coordinates2ID = {}
for i in range(len(points)):
  points[i].properties = { 'point_id': i+1 }
  coords = points[i]['coordinates']
  coordinates2ID[(coords[0], coords[1])] = i+1
  nodes.append(i+1)

print("Total number of nodes scraped: ", len(nodes))

# format edges
edges = { src: [] for src in nodes }
for shape in shapes:
  coords = [ tuple(p) for p in shape['geometry']['coordinates'] ]
  if coords[0] in coordinates2ID and coords[1] in coordinates2ID:
    src = coordinates2ID[coords[0]]
    dst = coordinates2ID[coords[1]]
    edges[src].append(dst)
    edges[dst].append(src) # bi-directional graph

# put edges in the points data so that we can find out neighbors of a point
# in the client data.
for i in nodes:
  points[i-1].properties['neighbors'] = edges[i]

# Print out JSON object for client
client = { 'features': points }
clientFile = open(CLIENT_OUTPUT_PATH, 'w')
clientFile.write(json.dumps(client))
clientFile.close()


############################ ALL PAIRS SHORTEST PATHS USING BFS
UNREACHABLE_COUNT = 0
table = []
for src in nodes:
  jumps = { src: src }
  BFS = Queue()
  visited = { src }

  # direct neighbors of src
  for n in edges[src]:
    jumps[n] = n
    BFS.put((n, n))
    visited.add(n)

  # BFS
  while not BFS.empty():
    dst, jump = BFS.get()
    for n in edges[dst]:
      if n not in visited:
        jumps[n] = jump
        visited.add(n)
        BFS.put((n, jump))

  # Format as table
  for dst in nodes:
    if src == dst:
      continue

    next = jumps.get(dst, 0)
    if next == 0:
      UNREACHABLE_COUNT = UNREACHABLE_COUNT + 1

    key = [src, dst]
    value = [src, dst, next]
    table.append([key, value])

# write out JSON object for server
serverFile = open(SERVER_OUTPUT_PATH, 'w')
serverFile.write(json.dumps(table))
serverFile.close()

print("Total size of table", len(table))
print("Unreachable pairs", UNREACHABLE_COUNT)
