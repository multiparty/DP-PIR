/* global UI, hash, config */
(function () {
  const bufferToNumber = function (buffer) {
    // Little endian...
    let result = 0;
    for (let i = buffer.length - 1; i >= 0; i--) {
      offset = i * 8;
      result += (buffer[i] << offset);
    }
    return result;
  };

  // Sends a query over a web socket, and returning a promise to the service's
  // response.
  const makeQuery = function (queryValue) {
    return new Promise(function (resolve) {
      let socket = new WebSocket('ws://localhost:3000/');
      socket.addEventListener('open', function () {
        // Incremental sharing of query.
        const shares = window.incrementalShareGenerate(queryValue, config.parties);

        // Additive sharing of 0.
        const preshares = window.additiveShareGenerate(0, config.parties + 1);
        const storedPreshare = preshares.pop();
        console.log(shares);
        console.log(preshares);

        // Onion encrypt shares.
        const encrypted = window.OnionEncrypt(shares, preshares, config);

        // Make query object.
        const query = new Uint8Array(8 + encrypted.length);
        // Little endian encoding of tally = 1.
        query.set([1, 0, 0, 0, 0, 0, 0, 0], encrypted.length);
        query.set(encrypted, 0);

        // Handle the response
        socket.onmessage = async function (event) {
          socket.close();
          // De-serialize response.
          const bytes = await event.data.arrayBuffer();
          const response = bufferToNumber(new Uint8Array(bytes));
          const value = window.additiveShareReconstruct(response, storedPreshare);
          resolve(value);
        };

        socket.send(query);
      });
    });
  };

  // Main entry point to our protocol: makes repeated queries until destination
  // is reached.
  window.protocol = async function (src, dst) {
    const query = hash([src, dst], UI.TOTAL_COUNT);
    const response = await makeQuery(query);

    // Find out what the response mean (search for hash).
    // I.e. transform response to the next hop point id!
    const srcPoint = UI.getPointById(src);
    const neighbors = srcPoint.properties['neighbors'].slice();
    neighbors.push(0);  // Special case: unreachable destinations.

    // Hash search.
    let next = null;
    for (let i = 0; i < neighbors.length; i++) {
      const candidateResponse = [src, dst, neighbors[i]];
      if (response === hash(candidateResponse, UI.TOTAL_COUNT)) {
        next = neighbors[i];
      }
    }

    // Unrecognized hash!
    if (next == null) {
      throw new Error(
        'Internal Error: response returned unrecognizable hash ' + response);
    }

    // Is the destination unreachable?
    if (next === 0) {
      alert('Destination unreachable!');
      return;
    }

    // Draw path!
    UI.drawPath([src, next]);
    if (next !== dst) {  // Keep going.
      window.protocol(next, dst);
    }
  };
})();
