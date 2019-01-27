/* global mySodiumWrapper, BN */
(function (exports, node) {
  var ECWrapper, _BN;
  if (node) {
    ECWrapper = require('../../lib/libsodium-port/wrapper.js');
    _BN = require('bn.js');
  } else {
    ECWrapper = mySodiumWrapper;
    _BN = BN;
  }

  // returns an array with the first element being point * m, and the remaining elements
  // being multiplicative shares of m^-1 % prime
  // point is already represented as a Uint8Array/array
  exports.share = function (point, party_count) {
    var masks = [];
    var mult = new _BN(1);
    for (var i = 0; i < party_count - 1; i++) {
      var mask = ECWrapper.randomScalar(); // already generated uniformly mod prime
      masks.push(mask);
      mult = mult.mul(mask).mod(ECWrapper.prime);
    }
    masks.push(mult.invm(ECWrapper.prime));

    masks[0] = ECWrapper.scalarMult(new Uint8Array(point), ECWrapper.BNToBytes(masks[0]));

    // Dump shares to JSONify representation
    masks[0] = Array.from(masks[0]);
    for (i = 1; i < masks.length; i++) {
      masks[i] = masks[i].toString();
    }
    return masks;
  };

  // shares is an array, with the first element being point * m, and the remaining
  // elements being a multiplicative share of m^-1 % prime
  exports.reconstruct = function (shares) {
    var unmask = new _BN(1);
    for (var i = 1; i < shares.length; i++) {
      var share = new _BN(shares[i]);
      unmask = unmask.mul(share).mod(ECWrapper.prime);
    }

    var result = ECWrapper.scalarMult(new Uint8Array(shares[0]), ECWrapper.BNToBytes(unmask));
    return Array.from(result);
  }
}((typeof exports === 'undefined' ? this.ECMPC = {} : exports), typeof exports !== 'undefined'));