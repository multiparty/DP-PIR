/*
 * Backend server:
 * 1. Establishes a jiff computation/instance with the other frontend servers
 * 2. Runs a web-server that exposes an API for computing/recomputing all pairs shortest paths locally.
 * 3. Executes the pre-processing protocol with frontend servers (oblivious shuffle + collision free PRF) on the all pairs shortests paths every time they are computed.
 * 4. When a frontend server demands: executes retrival protocol (local access by index).
 */

// Read Configs
const DEFAULT_MAP_DATA_PATH = 'data/server-map.json';

// Initialize Express and JIFF Instance
var party = require('./party.js');
require('./protocols/preprocessing.js').backEnd(party);

// Data
var recompute_count = 0; // Keeps track of how many times we recomputed.
var garbled_tables = []; // Maps index i to the resulting encrypted table of the ith re-computation.
var preprocess_requests = {}; // Remember requests and respond to them after tasks are done

// Routes and Functionality

// when http://localhost:8080/recompute/[<input>] is called,
// server recomputes shortest paths according to what is
// defined in the file: ./<input> (could be a path)
party.app.get('/recompute/:input?', function (req, res) {
  if (party.jiff.id !== 1) {
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
  party.jiff.emit('begin_preprocess', party.config.ids[party.config.BACKEND_LEADER].slice(1), msg, false);

  setTimeout(function () { // timeout to ensure emits are sent out first
    party.begin_preprocess(party.jiff.id, msg);
  }, 200);
});

// Start sending the table to frontend parties in a ring.
party.jiff.listen('begin_preprocess', party.begin_preprocess);

// Table came back to us after finishing all the ring!
party.jiff.listen('preprocess', function (sender_id, data) {
  var result = party.end_preprocess(sender_id, data);
  recompute_count = result.number;
  garbled_tables[result.number] = result.garbled_table;
  if (result.number > 3) {
    delete garbled_tables[result.number - 3];
  }

  // Reply to pre-process request
  if (party.jiff.id === 1) {
    preprocess_requests[result.number].json({success: true});
  }
});

// Listen to queries from frontends
party.jiff.listen('query', query_from_party);


// Listen to queries from user
party.app.get('/query/:number/:src_dest', query_from_user);


function query_from_user() {
  console.log('Query from user');
}

function query_from_party() {
  console.log('Query from party');
}
