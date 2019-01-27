/*
 * Frontend server:
 * 1. Establishes a jiff computation/instance with the other servers.
 * 2. Runs a web-server that exposes an API for user to query.
 * 3. Executes the pre-processing protocol (oblivious shuffle + collision free PRF) on data provided by backend server.
 * 4. Garbles and De-garbles queries of users under MPC.
 */

// Initialize Express and JIFF Instance
var party = require('./party.js');

// Routes and Functionality

// the backend leader wants us to pre-process!
// Call pre-process with the given re-computation number to
// replicate keys and start pre-processing when data is received.
party.jiff.listen('start pre-processing', function (_, msg) {
  var recompute_number = parseInt(msg);
  party.protocols.preprocess(recompute_number);
});

// Listen to queries from frontends/backends
party.jiff.listen('query', query_from_party);

// Listen to queries from user
party.app.get('/query/:number/:src_dest', query_from_user);

function query_from_user() {
  console.log('Query from user');
}

function query_from_party() {
  console.log('Query from party');
}