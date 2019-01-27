// Preprocessing protocol:
//
const keysHelper = require('../helpers/keys.js');
const garbleHelper = require('../helpers/garble.js');

const replicateKey = async function (party, tag) {
  // If this is the leader, it must generate and broadcast key
  var replica_ids = party.config.ids[party.config.owner];
  if (party.jiff.id === replica_ids[0]) {
    party.protocols.broadcast.broadcast(tag + ':key', keysHelper.generate());
  }

  // Receive key that was broad-casted by leader
  var key = await party.protocols.broadcast.get(tag + ':key');

  // Parse and store key
  key = keysHelper.parse(key);
  party.keys[tag] = key;
  party.invKey[tag] = keysHelper.inverse(key[1]);
  return key;
};

// Frontend protocol
module.exports = function (party) {
  party.protocols.preprocess = async function (tag, table) {
    var key = await replicateKey(party, tag);

    // if the table is given as a parameter (for backends) use it
    // otherwise, expect it to be forwarded from previous party (for frontends).
    if (party.config.owner > 1) {
      table = await party.protocols.forward.get(tag + ':forward');
    }

    // Logging and timing
    console.log(party.jiff.id, 'Begin Core Pre-processing #', tag, 'size', table.length);
    var start_time = new Date().getTime();

    // Garble the table (in place).
    garbleHelper.garbleTable(table, key);

    // Shuffle the table
    table = await party.protocols.shuffle(tag + ':shuffle', table);

    // Logging and timing
    var end_time = new Date().getTime();
    console.log(party.jiff.id, 'Finished Core Pre-processing #', tag, (end_time - start_time) / 1000);

    // Move table forward to next party
    if (party.config.owner < party.config.owner_count) {
      party.protocols.forward.forward(tag + ':forward', table);
    } else {
      // last front end sends a copy of its garbled chunk of the table to EVERY backend
      // so that every backend can answer any query.
      party.protocols.forward.toAll(tag + ':final', table);
    }

    // If this is a backend server, it must wait for the table to make
    // a full cycle through all frontends, and back to it.
    if (party.config.owner === 1) {
      return await party.protocols.forward.getAll(tag + ':final');
    }
  };
};