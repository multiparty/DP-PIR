var ECWrapper = require('../../lib/libsodium-port/wrapper.js');
var bn = require('bn.js');

module.exports = {
  generate: function () {
    // BN as a string
    var key1 = ECWrapper.randomScalar().toString(); // already generated mod prime
    var key2 = ECWrapper.randomScalar().toString();
    return [key1, key2];
  },
  parse: function (key) {
    var key1 = key[0];
    var key2 = key[1];

    var bnk1 = new bn(key1);
    var bnk2 = new bn(key2);

    key1 = ECWrapper.BNToBytes(bnk1);
    key2 = ECWrapper.BNToBytes(bnk2);
    return [{ bytes: key1, bn: bnk1 }, { bytes: key2, bn: bnk2 }];
  },
  inverse: function (singleKey) {
    var bnKey = singleKey.bn;
    var invKey = bnKey.invm(ECWrapper.prime);
    return { bytes: ECWrapper.BNToBytes(invKey), bn: invKey };
  }
};