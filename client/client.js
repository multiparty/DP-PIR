/* global protocol, drawPath */

function oneStep(src, dst) {
  return protocol(src, dst).then(function (next) {
    // Is the destination unreachable?
    if (next === 0) {
      alert("Destination unreachable!");
      drawPath([src, dst]);
      return false;
    }

    // Draw path!
    drawPath([src, next]);

    // Keep going.
    if (next == dst) {
      return true;
    }    
    return oneStep(next, dst);
  });
}

// eslint-disable-next-line no-unused-vars
function pointsSelected() {
  const src = window.localStorage.getItem('StartPointId');
  const dst = window.localStorage.getItem('StopPointId');

  oneStep(src, dst).catch(function (error) {
    alert("Internal error!");
    console.log(error);
  });
}
