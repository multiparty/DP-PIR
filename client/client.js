/* global protocol, drawPath */

function oneStep(src, dst) {
  if (src === dst) {
    return;
  }

  return protocol(src, dst).then(function (next) {
    // Is the destination unreachable?
    if (next === 0) {
      alert("Destination unreachable!");
      drawPath([src, dst]);
      return;
    }

    // Draw path!
    drawPath([src, next]);

    // Keep going.
    if (next != dst) {
      oneStep(next, dst);
    }    
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
