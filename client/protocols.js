/* global UI, hash */
(function () {
  // Sends a query over a web socket, and returning a promise to the service's
  // response.
  const makeQuery = function (query) {
    return new Promise(function (resolve) {
      let socket = new WebSocket('ws://localhost:3000/');
      socket.onmessage = function (event) {
        socket.close();
        resolve(parseInt(event.data));
      };
      socket.addEventListener('open', function () {
        socket.send(query.toString());
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
