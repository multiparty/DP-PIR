/* global TOTAL_COUNT, getPointById */
let prime = 27644437;

const mod = function(a, modulus){
  return a % modulus > 0 ? a : a + modulus;
}

const incrementalShareGenerate = function(query, numparty){
  let shares = [];

  let t = 1;
  for (let i = 0; i < numparty - 1; i++) {
    let share = new Object()
    let x = Math.random() % prime;
    let y = Math.random() % prime;
    t = (t * y + x) % prime;
    share.x = x;
    share.y = y;
    shares.push(share);
  }
  let share = new Object();
  let last_y = Math.random() % prime;
  let last_x = mod(query - t * last_y, prime);
  share.x = last_x;
  share.y = last_y;
  shares.push(share);

  return shares;

}

const additiveShareGenerate = function(query, numparty){
  let shares = [];

  let t = 0;
  for (let i = 0; i < numparty - 1; i++) {
    let x = Math.random() % prime;
    t = t + x % prime;
    shares.push(x);
  }
  let last_x = mod(query - t, prime);
  shares.push(last_x);

  return shares;

}

const additiveShareReconstruct= function(tally, share){
  return (tally + share) % prime;
}


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
