var ECWrapper = require('../../lib/libsodium-port/wrapper.js');
var bn = require('bn.js');

module.exports = {
  generate: function () {
    // BN as a string
    var key1 = ECWrapper.randomScalar().toString(); // already generated mod prime
    var key2 = ECWrapper.randomScalar().toString();
    var key = [key1, key2];
    return key;
  },
  parse: function (key) {
    var key1 = key[0];
    var key2 = key[2];

    key1 = ECWrapper.BNToBytes(new bn(key1));
    key2 = ECWrapper.BNToBytes(new bn(key2));
    return [key1, key2];
  },
  inverse: function (singleKey) {
    var bnKey = ECWrapper.bytesToBN(singleKey);
    var invKey = bnKey.invm(ECWrapper.prime);
    return ECWrapper.BNToBytes(invKey);
  }
};