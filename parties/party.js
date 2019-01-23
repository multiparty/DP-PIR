// Includes shared code and functionality between frontend and backend parties!
// Modeled as a library where a frontend or backend party calls one or more of the provided functions
// at the right time in the execution / protocols.
// The main different in frontend and backend parties is not the code they execute, but rather when they execute it!
// Frontend and backend parties are both event-driven, with different events coming in either from the end-users
// or the different parties.
// Check parties/frontend.js or parties/backend.js for the code of frontend and backend parties respectively.

// Dependencies
var express = require('express');
var jiff_client = require('../jiff/lib/jiff-client');
var jiff_client_bignumber = require('../jiff/lib/ext/jiff-client-bignumber');
var ECWrapper = require('../lib/libsodium-port/wrapper.js');
var BN = require('bn.js');

// Configurations!
var config = require('./config.js');



// Express Configuration
var app = express();

app.use(function (req, res, next) { // Cross Origin Requests Allowed
  res.header('Access-Control-Allow-Origin', '*');
  res.header('Access-Control-Allow-Credentials', 'true');
  res.header('Access-Control-Allow-Methods', 'OPTIONS, GET, POST');
  res.header('Access-Control-Allow-Headers', 'Content-Type, Depth, User-Agent, X-File-Size, X-Requested-With, If-Modified-Since, X-File-Name, Cache-Control');
  next();
});

// JIFF Configuration
var options = {
  // computation meta-data
  party_id: config.id,
  party_count: config.all_parties.length,
  autoConnect: false,
  initialization: {
    owner_party: config.owner
  },
  onConnect: function (jiff_instance) { // Connection
    var port = config.base_port + jiff_instance.id;
    app.listen(port, function () { // Start listening with express on port 9111
      console.log('Party up and listening on ' + port);
    });
  }
};

// Connect JIFF
var jiff_instance = jiff_client.make_jiff('http://localhost:3000', 'shortest-path-1', options);
jiff_instance.apply_extension(jiff_client_bignumber, options);
jiff_instance.connect();



// Shared functionality
var SRC_DEST_PAIR = 0, NEXT_HOP = 1;
var recompute_number = 0; // Track the most recent install recompute number (latest version of the table/keys)
var keys = {}; // map recompute number to [ <column1_key>, <column2_key> ]

// Main preprocessing functionality
var preprocess = function (table, number) {
  console.log(jiff_instance.id, 'begin preprocess #', number, 'size', table.length);
  var startTime = new Date().getTime();

  // Generate keys
  var key1 = ECWrapper.BNToBytes(ECWrapper.randomScalar());
  var key2 = ECWrapper.BNToBytes(ECWrapper.randomScalar());
  keys[number] = [ key1, key2 ];

  // in place garbling
  var lookup = {};
  for (var i = 0; i < table.length; i++) {
    var entry = table[i];
    entry[SRC_DEST_PAIR] = Array.from(ECWrapper.scalarMult(new Uint8Array(entry[SRC_DEST_PAIR]), key1));

    var str = entry[NEXT_HOP].toString();
    if (lookup[str] == null) {
      entry[NEXT_HOP] = Array.from(ECWrapper.scalarMult(new Uint8Array(entry[NEXT_HOP]), key2));
      lookup[str] = entry[NEXT_HOP];
    }
  }

  var endTime = (new Date().getTime() - startTime) / 1000;
  console.log(jiff_instance.id, 'preprocess #', number, 'completed in ', endTime, 'seconds');

  // send to the next party
  var msg = JSON.stringify({ table: table, recompute_number: number });
  var nextId = (jiff_instance.id + config.ids[config.owner].length);
  if (nextId > config.all_parties.length) {
    nextId = nextId - config.all_parties.length;
  }

  jiff_instance.emit('preprocess', [ nextId ], msg, false);
};



// Exports
module.exports = {
  // variables
  app: app,
  jiff_instance: jiff_instance,
  config: config,
  // functions
  preprocess: preprocess
};
