/* global sodium */
(function () {
  const numberToBytes = function (x) {
    let bitString = x.toString(2);
    while (bitString.length < 64) {
      bitString = '0' + bitString;
    }

    const bytes = [];
    for (let i = 0; i < 8; i++) {
      const start = i * 8;
      const end = (i + 1)  * 8;
      bytes.push(parseInt(bitString.substring(start, end), 2));
    }

    return bytes.reverse();  // Encode in LITTLE ENDIAN for x86!!!!
  };

  window.OnionEncrypt = function (shares, preshares, config) {
    let onion = new Uint8Array(0);
    for (let i = shares.length - 1; i >= 0; i--) {
      const x = shares[i].x;
      const y = shares[i].y;
      const preshare = preshares[i];

      // Decode the share into bytes, same format as c++ side struct.
      const bytes = numberToBytes(x).concat(numberToBytes(y)).concat(numberToBytes(preshare));
      for (let i = 0; i < onion.length; i++) {
        bytes.push(onion[i]);
      }

      // Decode base64 encoded public key.
      const base64PK = window.atob(config.keys[i + 1].public_key);
      const pk = new Uint8Array(32);
      for (let i = 0; i < Math.min(base64PK.length, pk.length); i++) {
        pk[i] = base64PK.charCodeAt(i);
      }

      onion = sodium.crypto_box_seal(Uint8Array.from(bytes), pk);
    }

    return onion;
  };

})();
