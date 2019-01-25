// express config
var path = require('path');

var express = require('express');
var app = express();

app.use(function (req, res, next) {
  res.header('Access-Control-Allow-Origin', '*');
  res.header('Access-Control-Allow-Credentials', 'true');
  res.header('Access-Control-Allow-Methods', 'OPTIONS, GET, POST');
  res.header('Access-Control-Allow-Headers', 'Content-Type, Depth, User-Agent, X-File-Size, X-Requested-With, If-Modified-Since, X-File-Name, Cache-Control');
  next();
});

// Serve static files.
app.use('/static', express.static(path.join(__dirname, 'static/')));
app.use('/helpers', express.static(path.join(__dirname, 'parties/helpers/')));
app.use('/jiff', express.static(path.join(__dirname, 'jiff/lib')));
app.use('/node_modules', express.static(path.join(__dirname, 'node_modules/')));

app.get('/client.js', function (req, res) {
  res.sendFile(__dirname + '/client.js');
});

app.get('/data/client-map.js', function (req, res) {
  res.sendFile(__dirname + '/data/client-map.js');
});

app.get('/', function (req, res) {
  res.sendFile(__dirname + '/index.html');
});


// JIFF config
var jiff_instance;
var http = require('http').Server(app);

var config = require('./parties/config/config.json');
var parties = config.parties; // number of parties
var replicas = config.replicas; // number of machines per party
var total = parties * replicas; // total number of machines

// Hooks for JIFF
var beforeInitHook = function (jiff, computation_id, msg, meta) {
  if (meta.party_id != null) {
    // if party is requesting a specific id, we do not do anything (maybe it is reconnecting?)
    return meta;
  }

  var spare_ids = jiff_instance.spare_party_ids[computation_id];

  // we won't have parties try to reserve particular ids, instead
  // they will provide their owner, 1 for backend, 2...n for front-end parties.
  var owner = msg['owner_party'];
  if (typeof(owner) !== 'number' || owner < 1 || owner > parties) {
    throw new Error('Invalid owner!');
  }

  // We will assign any id out of the available ones for this owner!
  var party_id = null;
  for (var i = 1; i <= replicas; i++) {
    var candidate = (owner - 1) * replicas + i;
    if (spare_ids.is_free(candidate)) {
      party_id = candidate;
      break;
    }
  }

  // If we could not find any free ID, then all replicas for the given owner are already reserved!
  if (party_id == null) {
    throw new Error('All owner\'s replicas are reserved!');
  }

  return { party_id: party_id, party_count: total };
};

var options = {
  logs: false,
  hooks: {
    beforeInitialize: [ beforeInitHook ]
  }
};

// Create jiff Instance
jiff_instance = require('./jiff/lib/jiff-server').make_jiff(http, options);
jiff_instance.apply_extension(require('./jiff/lib/ext/jiff-server-bignumber'), options);


// All done, start listening
http.listen(3000, function () {
  console.log('listening on *:3000');
});
