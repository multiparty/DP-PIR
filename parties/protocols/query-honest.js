// Honest client query protocol:
// Client secret-shares SRC_DEST query point P into P * m1, m2, ..., m_p such that m1 * ... * m_p = 1 mode prime
// Client sends each share to a party replica with P * m1 sent to a backend replica
// Frontend replicas multiply m_i by their SRC_DEST key mode prime, and send it to backend
// Backend multiplies its key with all received scalars mod prime, and scalar multiplies it with p * m1.
// The resulting point q matches syntactically with one row in the garbled table, backend returns a secret sharing
// of the associated garbled NEXT_HOP. De-garbling is symmetrical with front-ends multiplying with inverse of
// their NEXT_HOP keys and sending results to client.
const ECWrapper = require('../../lib/libsodium-port/wrapper.js');
const shareHelper = require('../helpers/ECMPC.js');

const SRC_DEST_PAIR_INDEX = 0;
const NEXT_HOP_INDEX = 1;

var bn = require('bn.js');
var keyMulMod = function (scalar1, scalar2) {
  scalar1 = new bn(scalar1);
  scalar2 = scalar2.bn;
  var result = scalar1.mul(scalar2).mod(ECWrapper.prime);
  return result.toString();
};

// Frontend protocol
module.exports = function (party) {
  party.protocols.query_honest = async function (tag, share, tableHelper) {
    var recompute_number = party.current_recompute_number;

    var key = party.keys[recompute_number];
    var invKey = party.invKey[recompute_number];
    var clique = party.config.cliqueNoSelf;

    // Backend: share is an Array representing an EC Point
    if (party.config.owner === 1) {
      // Query Garbling
      var shares = await party.protocols.chunk.combine(clique, tag + ':honest_query');
      shares[0] = keyMulMod(shares[0], key[SRC_DEST_PAIR_INDEX]);
      var garbledQuery = shareHelper.reconstruct([share].concat(shares));

      // Lookup in table
      var garbledHop = tableHelper.lookup(garbledQuery, recompute_number);

      // Next Hop De-Garbling
      shares = shareHelper.share(garbledHop, clique.length + 1);
      shares[1] = keyMulMod(shares[1], invKey);
      party.protocols.chunk.chunk(clique, tag + ':honest_reply', shares.slice(1));
      return shares[0]; // this goes to the client
    }

    // Front end: share is a string representing a bn.

    // Garbling round
    share = keyMulMod(share, key[SRC_DEST_PAIR_INDEX]);
    party.protocols.chunk.chunk([ clique[0] ], tag + ':honest_query', [ share ]);

    // De-Garbling round
    var reply = await party.protocols.chunk.combine([ clique[0] ], tag + ':honest_reply');
    share = keyMulMod(reply[0], invKey);
    return share;
  };
};