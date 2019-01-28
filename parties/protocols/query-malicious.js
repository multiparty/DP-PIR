// Malicious client query protocol:
// Client secret-shares SRC_DEST query point P into P * m1, m2, ..., m_p such that m1 * ... * m_p = 1 mode prime
// and sends each share to a replica as in honest_query.
// However, replica of first frontend party receives the point share, and it passes it along in a chain
// through the rest of the frontends up to the backend.
// Each server scalar multiplies the point by its key and share.
const ECWrapper = require('../../lib/libsodium-port/wrapper.js');
const shareHelper = require('../helpers/ECMPC.js');

const SRC_DEST_PAIR_INDEX = 0;
const NEXT_HOP_INDEX = 1;

var bn = require('bn.js');
var keyMulMod = function (scalar1, scalar2) {
  scalar1 = new bn(scalar1);
  scalar2 = scalar2.bn;
  return scalar1.mul(scalar2).mod(ECWrapper.prime);
};

var shareIdentity = function () {
  var scalar1 = ECWrapper.randomScalar();
  var scalar2 = scalar1.invm(ECWrapper.prime);
  return [scalar1, scalar2];
};

// Frontend protocol
module.exports = function (party) {
  const clique = party.config.cliqueNoSelf;

  const getGarblingShares = async function (tag, clientData) {
    if (party.config.owner === 2) {
      // first front end only receives client data
      return { point: JSON.parse(clientData), scalar: new bn(1) };
    } else {
      var point = await party.protocols.forward.get(tag + ':malicious_query');
      return { point: point, scalar: new bn(clientData) };
    }
  };

  const getDeGarblingShares = async function (tag) {
    var reply = await party.protocols.chunk.combine([ clique[0] ], tag + ':malicious_reply:chunk');
    reply = reply[0];

    if (party.config.owner === party.config.owner_count) {
      // first front end only receives client data
      return { point: reply, scalar: new bn(1) };
    } else {
      var point = await party.protocols.forward.get(tag + ':malicious_reply');
      return { point: point, scalar: new bn(reply) };
    }
  };

  party.protocols.query_malicious = {};
  party.protocols.query_malicious.frontend = async function (tag, clientData) {
    var shares, scalar, garbledPoint, deGarbledPoint, identity;
    var recompute_number = party.current_recompute_number;

    const key = party.keys[recompute_number];
    const invKey = party.invKey[recompute_number];

    // Get all needed input shares
    shares = await getGarblingShares(tag, clientData);

    // Garble the query
    identity = shareIdentity();
    scalar = keyMulMod(shares.scalar.mul(identity[0]), key[SRC_DEST_PAIR_INDEX]);
    garbledPoint = ECWrapper.scalarMult(new Uint8Array(shares.point), ECWrapper.BNToBytes(scalar));
    garbledPoint = Array.from(garbledPoint);

    // Forward garbling shares
    party.protocols.forward.forward(tag + ':malicious_query', garbledPoint);
    party.protocols.chunk.chunk([ clique[0] ], tag + ':malicious_query:chunk', [ identity[1].toString() ]);

    // Receive all needed input for degarbling
    shares = await getDeGarblingShares(tag);

    // De-Garble the response
    if (party.config.owner !== 2) {
      identity = shareIdentity();
      shares.scalar = shares.scalar.mul(identity[0]);
    }

    scalar = keyMulMod(shares.scalar, invKey);
    deGarbledPoint = ECWrapper.scalarMult(new Uint8Array(shares.point), ECWrapper.BNToBytes(scalar));
    deGarbledPoint = Array.from(deGarbledPoint);

    // Forward DeGarbling shares
    if (party.config.owner !== 2) {
      party.protocols.forward.forward(tag + ':malicious_reply', deGarbledPoint, true);
      return identity[1].toString();
    } else {
      return deGarbledPoint;
    }
  };


  party.protocols.query_malicious.backend = async function (tag, clientData, tableHelper) {
    var shares, garbledPoint, garbledHop, combine, clientShare, frontendShares;
    var recompute_number = party.current_recompute_number;

    const key = party.keys[recompute_number];
    const invKey = party.invKey[recompute_number];

    // Get all needed input shares
    shares = await getGarblingShares(tag, clientData);
    combine = await party.protocols.chunk.combine(clique, tag + ':malicious_query:chunk');

    // Garble the query
    combine.push(keyMulMod(shares.scalar, key[SRC_DEST_PAIR_INDEX]));
    garbledPoint = shareHelper.reconstruct([shares.point].concat(combine));

    // Lookup in table
    garbledHop = tableHelper.lookup(garbledPoint, recompute_number);

    // Next Hop De-Garbling
    shares = shareHelper.share(garbledHop, clique.length + 1);
    clientShare = keyMulMod(shares[1], invKey).toString();
    frontendShares = shares.slice(2).concat([shares[0]]);

    party.protocols.chunk.chunk(clique, tag + ':malicious_reply:chunk', frontendShares);
    return clientShare;
  };
};