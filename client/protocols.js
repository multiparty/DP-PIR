/* global TOTAL_COUNT, getPointById */


const BFS = function (src, dst) {
  const queue = [[src, 0]];
  const visited = {src: true};
  while (queue.length > 0) {
    const entry = queue.shift();
    const n = entry[0];
    const c = entry[1];
    if (n == dst) {
      return c;
    }

    const neighbors = getPointById(n).properties["neighbors"];
    for (let d of neighbors) {
      if (!visited[d]) {
        queue.unshift([d, c+1]);
        visited[d] = true;
      }
    }
  }

  // Unreachable.
  return Number.MAX_VALUE;
}

const makeQuery = function (src, dst, query) {
  return new Promise(function (resolve) {
    if (src == dst) {
      resolve(-1);
    }

    const srcPoint = getPointById(src);
    let min = Number.MAX_VALUE;
    let next = 0;
    for (let i = 0; i < srcPoint.properties["neighbors"].length; i++) {
      const neighbor = srcPoint.properties["neighbors"][i];
      const cost = BFS(neighbor, dst);
      if (cost < min) {
        next = neighbor;
        min = cost;
      }
    }
    resolve(hash([src, dst, next], TOTAL_COUNT));
  });
}

const protocol = function (src, dst) {
  const query = hash([src, dst], TOTAL_COUNT);
  return makeQuery(src, dst, query).then(function (response) {
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

    throw new Error("Internal Error: response returned unrecognizable hash "
      + response);
  });
}

