/*
 * Frontend server:
 * 1. Establishes a jiff computation/instance with the other servers.
 * 2. Runs a web-server that exposes an API for user to query.
 * 3. Executes the pre-processing protocol (oblivious shuffle + collision free PRF) on data provided by backend server.
 * 4. Garbles and De-garbles queries of users under MPC.
 */

// Initialize Express and JIFF Instance
var party = require('./party.js');
require('./protocols/preprocessing.js').frontEnd(party);

// Routes and Functionality

// backend or previous frontend wants us to preprocess!
// we preprocess and then forward to next frontend.
party.jiff.listen('preprocess', function (_, msg) {
  msg = JSON.parse(msg);
  party.preprocess(msg.table, msg.recompute_number);
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