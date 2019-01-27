var ECWrapper = require('../../lib/libsodium-port/wrapper.js');

const SRC_DEST_PAIR_INDEX = 0;
const NEXT_HOP_INDEX = 1;

module.exports = {
  // in place garbling
  garbleTable: function (table, key) {
    var lookup = {};
    for (var i = 0; i < table.length; i++) {
      var entry = table[i];
      var src_dest_pair = entry[SRC_DEST_PAIR_INDEX];
      var next_hop = entry[NEXT_HOP_INDEX];

      // parse EC points
      src_dest_pair = new Uint8Array(src_dest_pair);
      next_hop = new Uint8Array(next_hop);

      // Garble by scalar mult
      var garbled1 = ECWrapper.scalarMult(src_dest_pair, key[SRC_DEST_PAIR_INDEX]);
      garbled1 = Array.from(garbled1);

      var next_hop_str = next_hop.toString();
      var garbled2 = lookup[next_hop_str];
      if (garbled2 == null) {
        garbled2 = ECWrapper.scalarMult(next_hop, key[NEXT_HOP_INDEX]);
        garbled2 = Array.from(garbled2);
        lookup[next_hop_str] = garbled2;
      }

      // Write answers back to table
      entry[SRC_DEST_PAIR_INDEX] = garbled1;
      entry[NEXT_HOP_INDEX] = garbled2;
    }
  }
};