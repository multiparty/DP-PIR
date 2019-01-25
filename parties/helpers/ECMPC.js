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
  // being multiplicative shares of m^-1 % Zp
  // point is already represented as a Uint8Array/array
  exports.share = function (point, Zp, party_count) {
    var masks = [];
    var mult = new _BN(1);
    for (var  i = 0; i < party_count - 1; i++) {
      var mask = ECWrapper.random(); // already generated uniformly mod Zp
      masks.push(mask);
      mult = mult.mul(mask).mod(Zp);
    }
    mask.push(mult.invm(Zp));

    masks[0] = ECWrapper.scalarMult(new Uint8Array(point), masks[0]);
    return masks;
  };

  // shares is an array, with the first element being point * m, and the remaining
  // elements being a multiplicative share of m^-1 % Zp
  exports.reconstruct = function (shares, Zp) {
    var unmask = new _BN(1);
    for (var i = 1; i < shares.length; i++) {
      unmask = unmask.mul(shares[i]).mod(Zp);
    }

    return ECWrapper.scalarMult(new Uint8Array(shares[0]), unmask);
  }
}((typeof exports === 'undefined' ? this.jiff = {} : exports), typeof exports !== 'undefined'));