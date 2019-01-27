/*
 * Backend server:
 * 1. Establishes a jiff computation/instance with the other frontend servers
 * 2. Runs a web-server that exposes an API for computing/recomputing all pairs shortest paths locally.
 * 3. Executes the pre-processing protocol with frontend servers (oblivious shuffle + collision free PRF) on the all pairs shortests paths every time they are computed.
 * 4. When a frontend server demands: executes retrival protocol (local access by index).
 */

// Initialize Express and JIFF Instance
var party = require('./party.js');

var tableHelper = require('./helpers/table.js');

// Backend specific code and functionality

// Begins the pre-processing cycle: load the table chunk from the data file
// start shuffling and garbling, send garbled data to the next party,
// wait for last party to send final garbled table, and install it.
const preprocess = async function (recompute_number) {
  try {
    // Chunk table
    var table = tableHelper.chunk(party.jiff.id, party.config);

    // Perform pre-processing
    var garbled_table = await party.protocols.preprocess(recompute_number, table);

    // Install table
    var noDuplicates = tableHelper.install(garbled_table, recompute_number);

    // Tell backend leader about installation status
    party.protocols.chunk.chunk([1], recompute_number + ':status', [noDuplicates]);
  } catch( ERR) { console.log(ERR); }
  return noDuplicates;
};

// Routes and Functionality

// when http://localhost:8080/recompute/[<input>] is called,
// the backend leader initiates a new pre-processing stage
if (party.jiff.id === 1) {
  party.app.get('/recompute', function (req, res) {
    console.log('Recomputation requested!');
    var startTime = new Date().getTime();

    // Tell all parties to be ready for pre-processing
    var recompute_number = party.current_recompute_number + 1;
    party.jiff.emit('start pre-processing', party.config.all_parties.slice(1), recompute_number.toString(), false);

    setTimeout(async function () { // timeout to ensure emits are sent out first
      var status = await preprocess(recompute_number);

      // Check if everyone successfully installed the table.
      var results = await party.protocols.chunk.combine(party.config.ids[party.config.owner], recompute_number + ':status');
      for (var i = 0; i < status.length; i++) {
        status = status && results;
      }

      // Clean previous tables and install
      tableHelper.clear(recompute_number - (status ? 3 : 0));
      if (party.current_recompute_number < recompute_number) {
        party.current_recompute_number = recompute_number;
      }

      // Update everyone with the status.
      var msg = JSON.stringify({ recompute_number: recompute_number, status: status });
      party.jiff.emit('install', party.config.all_parties.slice(1), msg, false);

      var endTime = new Date().getTime();
      res.json({ success: status, duration: (endTime - startTime) / 1000 });
    }, 200);
  });
}

// the backend leader wants us to pre-process!
// Call pre-process with the given re-computation number to
// replicate keys and start pre-processing when data is received.
party.jiff.listen('start pre-processing', function (_, msg) {
  var recompute_number = parseInt(msg);
  preprocess(recompute_number);
});

party.jiff.listen('install', function (sender_id, msg) {
  msg = JSON.parse(msg);
  if (msg['status'] && party.current_recompute_number < msg['recompute_number']) {
    party.current_recompute_number = msg['recompute_number'];
  }

  tableHelper.clear(msg['recompute_number'] - (msg['status'] ? 3 : 0));
});

// Listen to queries from user: the honest user query protocol
party.app.get('/query/honest/:tag/:src_dest', async function (req, res) {
  if (party.current_recompute_number === 0) {
    return res.status(500);
  }

  var tag = req.params['tag'];
  var pointShare = JSON.parse(req.params['src_dest']); // Array representing EC point

  pointShare = await party.protocols.query_honest(tag, pointShare, tableHelper);
  res.send(JSON.stringify({share: pointShare})); // Array representing EC point
});