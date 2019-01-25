// Shared code that reads the configurations for both backend and frontend servers.

// Read configurations
var config = require('./config.json');
var parties = config.parties; // number of parties
var replicas = config.replicas; // number of machines per party

// Exported Configuration variables
exports.base_port = config.base_port;
exports.owner = parseInt(process.argv[2]);
exports.replica = process.argv[3];
if (exports.replica !== '' && exports.replica != null) {
  exports.replica = parseInt(exports.replica);
  exports.id = (exports.owner - 1) * replicas + exports.replica;
} else {
  exports.replica = null;
  exports.id = null;
}

// Other configurations / constants
exports.BACKEND_LEADER = 1;
exports.SRC_DEST_PAIR_INDEX = 0;
exports.NEXT_HOP_INDEX = 1;

// map logical party/owner id to an array of actual party ids belonging to it
// e.g. for back-ends:
//      ids[BACKEND_LEADER] = [ 1, 2, 3, ...., 1 + replicas ] the ids of every backend replica!
exports.ids = {};
exports.all_parties = []; // a list of all ids in our computation
for (var own = 1; own <= parties; own++) {
  exports.ids[own] = [];
  for (var op = 1; op <= replicas; op++) {
    var id = (own - 1) * replicas + op;
    exports.ids[own].push(id);
    exports.all_parties.push(id)
  }
}