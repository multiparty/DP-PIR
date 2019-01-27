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
    for (var  i = 0; i < party_count - 1; i++) {
      var mask = ECWrapper.random(); // already generated uniformly mod prime
      masks.push(mask);
      mult = mult.mul(mask).mod(ECWrapper.prime);
    }
    mask.push(mult.invm(ECWrapper.prime));

    masks[0] = ECWrapper.scalarMult(new Uint8Array(point), masks[0]);
    return masks;
  };

  // shares is an array, with the first element being point * m, and the remaining
  // elements being a multiplicative share of m^-1 % prime
  exports.reconstruct = function (shares) {
    var unmask = new _BN(1);
    for (var i = 1; i < shares.length; i++) {
      unmask = unmask.mul(shares[i]).mod(ECWrapper.prime);
    }

    return ECWrapper.scalarMult(new Uint8Array(shares[0]), unmask);
  }
}((typeof exports === 'undefined' ? this.jiff = {} : exports), typeof exports !== 'undefined'));