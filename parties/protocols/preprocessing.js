var ECWrapper = require('../../lib/libsodium-port/wrapper.js');
//var BN = require('bn.js');

// Main pre-processing functionality: shared between all kinds of parties
var preprocess = function (party) {
  const jiff_instance = party.jiff;
  const keys = party.keys;
  const config = party.config;

  return function (table, number) {
    console.log(jiff_instance.id, 'begin preprocess #', number, 'size', table.length);
    var startTime = new Date().getTime();

    // Generate keys
    var key1 = ECWrapper.BNToBytes(ECWrapper.randomScalar());
    var key2 = ECWrapper.BNToBytes(ECWrapper.randomScalar());
    keys[number] = [key1, key2];

    // in place garbling
    var lookup = {};
    for (var i = 0; i < table.length; i++) {
      var entry = table[i];
      entry[config.SRC_DEST_PAIR_INDEX] = Array.from(ECWrapper.scalarMult(new Uint8Array(entry[config.SRC_DEST_PAIR_INDEX]), key1));

      var str = entry[config.NEXT_HOP_INDEX].toString();
      if (lookup[str] == null) {
        entry[config.NEXT_HOP_INDEX] = Array.from(ECWrapper.scalarMult(new Uint8Array(entry[config.NEXT_HOP_INDEX]), key2));
        lookup[str] = entry[config.NEXT_HOP_INDEX];
      }
    }

    var endTime = (new Date().getTime() - startTime) / 1000;
    console.log(jiff_instance.id, 'preprocess #', number, 'completed in ', endTime, 'seconds');

    // send to the next party
    var msg = JSON.stringify({table: table, recompute_number: number});
    var nextId = (jiff_instance.id + config.ids[config.owner].length);
    if (nextId > config.all_parties.length) {
      nextId = nextId - config.all_parties.length;
    }

    jiff_instance.emit('preprocess', [nextId], msg, false);
  };
};

// Backend specific pre-processing functionality
var begin_preprocess = function (party) {
  const config = party.config;

  return function (party_id, msg) {
    if (party_id !== config.BACKEND_LEADER) {
      return;
    }

    msg = JSON.parse(msg);

    // Array of entries, each entry on the form of [ <hash_src:dest_pair>, <hash_nexthop> ]
    // Each hash is a Uint8Array (of size 32).
    var table = require('../../' + msg.path);

    // Chunk table according to how many backend replicas
    var chunk_size = Math.floor(table.length / party.config.ids[config.BACKEND_LEADER].length);
    var chunk_index = chunk_size * (party.config.replica - 1);
    var chunk_end = table.length;
    if (party.config.replica < party.config.ids[config.BACKEND_LEADER].length) {
      chunk_end = chunk_index + chunk_size;
    }

    table = table.slice(chunk_index, chunk_end);
    party.preprocess(table, msg.recompute_number);
  };
};

// Backend specific post-processing functionality
var end_preprocess = function (party) {
  const config = party.config;

  return function (_, msg) {
    msg = JSON.parse(msg);

    var table = msg.table;
    var number = msg.recompute_number;

    // Convert Table to easy lookup format
    var duplicates = 0;
    var garbled_table = {};
    for (var i = 0; i < table.length; i++) {
      var key = table[i][config.SRC_DEST_PAIR_INDEX].toString();
      var val = new Uint8Array(table[i][config.NEXT_HOP_INDEX]);
      if (garbled_table[key] != null) {
        duplicates++;
      }
      garbled_table[key] = val;
    }

    // Duplicate detection: should always be 0
    console.log(party.jiff.id, ' preprocessing #', number, 'completed!');
    if (duplicates > 0) {
      console.log('WARNING: found', duplicates, 'duplicates');
    }

    // Install table
    return ({ number: number, garbled_table: garbled_table });
  };
};

module.exports = {
  backEnd: function (party) {
    party.begin_preprocess = begin_preprocess(party);
    party.preprocess = preprocess(party);
    party.end_preprocess = end_preprocess(party);
  },
  frontEnd: function (party) {
    party.preprocess = preprocess(party);
  }
};