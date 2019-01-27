const SERVER_DATA_PATH = '../../data/server-map.json';
const RAW_TABLE = require(SERVER_DATA_PATH);

const SRC_DEST_PAIR_INDEX = 0;
const NEXT_HOP_INDEX = 1;

// Backend specific data and functionality
var garbled_tables = {}; // Maps index i to the resulting encrypted table of the ith re-computation.

module.exports = {
  chunk: function (id, config) {
    // read table and chunk it
    var backends = config.ids[config.owner];
    var chunkSize = Math.ceil(RAW_TABLE.length / backends.length);
    var index = id - 1;

    return RAW_TABLE.slice(index * chunkSize, (index+1) * chunkSize);
  },
  install: function (table, recompute_number) {
    if (table.length !== RAW_TABLE.length) {
      console.log('length of garbled table ', table.length, 'does not match length of actual table', RAW_TABLE.length);
      return false;
    }

    var lookup = {};
    for (var i = 0; i < table.length; i++) {
      var key = table[i][SRC_DEST_PAIR_INDEX].toString();
      var val = new Uint8Array(table[i][NEXT_HOP_INDEX]);

      if (lookup[key] != null) {
        console.log('duplicates found in garbled table');
        return false;
      }

      lookup[key] = val;
    }

    garbled_tables[recompute_number] = lookup;
    return true;
  },
  clear: function (recompute_number) {
    delete garbled_tables[recompute_number];
  },
  lookup: function (garbledQuery, recompute_number) {
    return garbled_tables[recompute_number][garbledQuery.toString()];
  }
};