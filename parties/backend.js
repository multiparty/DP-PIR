/*
 * Backend server:
 * 1. Establishes a jiff computation/instance with the other frontend servers
 * 2. Runs a web-server that exposes an API for computing/recomputing all pairs shortest paths locally.
 * 3. Executes the pre-processing protocol with frontend servers (oblivious shuffle + collision free PRF) on the all pairs shortests paths every time they are computed.
 * 4. When a frontend server demands: executes retrival protocol (local access by index).
 */

// Read Configs
var DEFAULT_MAP_DATA_PATH = 'data/server-map.json';
var BACKEND_OWNER = 1;

// Initialize Express and JIFF Instance
var party = require('./party.js');

var app = party.app;
var jiff_instance = party.jiff_instance;
var config = party.config;

// Data
var SRC_DEST_PAIR = 0, NEXT_HOP = 1;
var recompute_count = 0; // Keeps track of how many times we recomputed.
var garbled_tables = []; // Maps index i to the resulting encrypted table of the ith re-computation.

// Remember requests and respond to them after tasks are done
var preprocess_requests = {};

// Routes and Functionality

// when http://localhost:8080/recompute/[<input>] is called,
// server recomputes shortest paths according to what is
// defined in the file: ./<input> (could be a path)
app.get('/recompute/:input?', function (req, res) {
  if (jiff_instance.id !== 1) {
    res.json({ success: false, error: 'party id is not 1!' });
    return;
  }

  console.log('Recomputation requested!');
  
  // Remember request to reply to later when preprocessing is done.
  preprocess_requests[recompute_count+1] = res;

  // parse request parameters
  var path = req.params.input != null ? req.params.input : DEFAULT_MAP_DATA_PATH;

  // Call begin_preprocess on all backend replicas
  // This executes the function `begin_preprocess' defined below
  // in all of the replicas (including this replica)!
  var msg = JSON.stringify({ path: path, recompute_number: recompute_count+1 });
  jiff_instance.emit('begin_preprocess', config.ids[BACKEND_OWNER].slice(1), msg, false);

  setTimeout(function () { // timeout to ensure emits are sent out first
    begin_preprocess(jiff_instance.id, msg)
  }, 200);
});

function begin_preprocess(_, msg) {
  msg = JSON.parse(msg);

  // Array of entries, each entry on the form of [ <hash_src:dest_pair>, <hash_nexthop> ]
  // Each hash is a Uint8Array (of size 32).
  var table = require('../' + msg.path);

  // Chunk table according to how many backend replicas
  var chunk_size = Math.floor(table.length / config.ids[BACKEND_OWNER].length);
  var chunk_index = chunk_size * (config.replica - 1);
  var chunk_end = table.length;
  if (config.replica < config.ids[BACKEND_OWNER].length) {
    chunk_end = chunk_index + chunk_size;
  }

  table = table.slice(chunk_index, chunk_end);
  party.preprocess(table, msg.recompute_number);
}

// Start sending the table to frontend parties in a ring.
jiff_instance.listen('begin_preprocess', begin_preprocess);

// Table came back to us after finishing all the ring!
jiff_instance.listen('preprocess', function (_, msg) {
  msg = JSON.parse(msg);

  var table = msg.table;
  var number = msg.recompute_number;

  // Convert Table to easy lookup format
  var duplicates = 0;
  var garbled_table = {};
  for (var i = 0; i < table.length; i++) {
    var key = table[i][SRC_DEST_PAIR].toString();
    var val = new Uint8Array(table[i][NEXT_HOP]);
    if (garbled_table[key] != null) {
      duplicates++;
    }
    garbled_table[key] = val;
  }

  // Duplicate detection: should always be 0
  console.log(jiff_instance.id, ' preprocessing #', number, 'completed!');
  if (duplicates > 0) {
    console.log('WARNING: found', duplicates, 'duplicates');
  }

  // Install table
  recompute_count = number;
  garbled_tables[number] = garbled_table;
  if (number > 3) {
    delete garbled_tables[number - 3];
  }

  // Reply to pre-process request
  preprocess_requests[number].json({ success: true });
});

// Listen to queries from frontends
jiff_instance.listen('query', query_from_party);


// Listen to queries from user
app.get('/query/:number/:src_dest', query_from_user);


function query_from_user() {
  console.log('Query from user');
}

function query_from_party() {
  console.log('Query from party');
}
