(function () {
  // 26 prime number (< sqrt(Number.MAX_SAFE_INTEGER))...
  let prime = 67108859;

  // Mod function that works correctly for negative values.
  const mod = function (a, modulus) {
    return a < 0 ? a % modulus + modulus : a + modulus;
  };

  /* Generate random numbers using the browser crypto libraries */
  const crypto_ = window.crypto || window.msCrypto;
  crypto_.__randomBytesWrapper = function (bytesNeeded) {
    var randomBytes = new Uint8Array(bytesNeeded);
    crypto_.getRandomValues(randomBytes);
    return randomBytes;
  };

  const random = function (max) {
    // Use rejection sampling to get random value within bounds
    // Generate random Uint8 values of 1 byte larger than the max parameter
    // Reject if random is larger than quotient * max (remainder would cause biased distribution), then try again

    // Values up to 2^53 should be supported, but log2(2^49) === log2(2^49+1), so we lack the precision to easily
    // determine how many bytes are required
    if (max > 562949953421312) {
      throw new RangeError('Max value should be smaller than or equal to 2^49');
    }

    var bitsNeeded = Math.ceil(Math.log(max)/Math.log(2));
    var bytesNeeded = Math.ceil(bitsNeeded / 8);
    var maxValue = Math.pow(256, bytesNeeded);

    // Keep trying until we find a random value within bounds
    while (true) { // eslint-disable-line
      var randomBytes = crypto_.__randomBytesWrapper(bytesNeeded);
      var randomValue = 0;

      for (var i = 0; i < bytesNeeded; i++) {
        randomValue = randomValue * 256 + (randomBytes.readUInt8 ? randomBytes.readUInt8(i) : randomBytes[i]);
      }

      // randomValue should be smaller than largest multiple of max within maxBytes
      if (randomValue < maxValue - maxValue % max) {
        return randomValue % max;
      }
    }
  };

  window.incrementalShareGenerate = function (query, numparty) {
    let shares = [];

    let t = 1;
    for (let i = 0; i < numparty - 1; i++) {
      let share = {};
      let x = random(prime);
      let y = random(prime);
      t = (t * y + x) % prime;
      share.x = x;
      share.y = y;
      shares.push(share);
    }

    let share = {};
    let last_y = random(prime);
    let last_x = mod(query - t * last_y, prime);
    share.x = last_x;
    share.y = last_y;
    shares.push(share);

    return shares;
  };

  window.additiveShareGenerate = function (query, numparty) {
    let shares = [];

    let t = 0;
    for (let i = 0; i < numparty - 1; i++) {
      let x = random(prime);
      t = t + x % prime;
      shares.push(x);
    }
    let last_x = mod(query - t, prime);
    shares.push(last_x);

    return shares;
  };

  window.additiveShareReconstruct = function (tally, share) {
    return (tally + share) % prime;
  };
})();
