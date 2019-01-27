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

// Listen to queries from user: the honest user query protocol
party.app.get('/query/honest/:tag/:scalar', async function (req, res) {
  if (party.current_recompute_number === 0) {
    return res.status(500);
  }

  var tag = req.params['tag'];
  var scalarShare = req.params['scalar']; // string representing bn

  scalarShare = await party.protocols.query_honest(tag, scalarShare); // string representing bn
  res.send(JSON.stringify({share: scalarShare}));
});