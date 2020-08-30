/* global TOTAL_COUNT, getPointById */
const makeQuery = function (query) {
  return new Promise(function (resolve) {
    let socket = new WebSocket("ws://localhost:3000/");
    socket.onmessage = function (event) {
      socket.close();
      resolve(parseInt(event.data));
    };
    socket.addEventListener('open', function () {
      socket.send(query.toString());
    });
  });
}

const protocol = function (src, dst) {
  const query = hash([src, dst], TOTAL_COUNT);
  return makeQuery(query).then(function (response) {
    // Find out what the response mean (search for hash).
    // I.e. transform response to the next hop point id!
    const srcPoint = getPointById(src);
    const neighbors = srcPoint.properties["neighbors"].slice();
    neighbors.push(0);  // Special case: unreachable destinations.

    let next = null;
    for (let i = 0; i < neighbors.length; i++) {
      const candidateResponse = [src, dst, neighbors[i]];
      if (response == hash(candidateResponse, TOTAL_COUNT)) {
        return neighbors[i];
      }
    }

    throw new Error(
        "Internal Error: response returned unrecognizable hash " + response);
  });
}

