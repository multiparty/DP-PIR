/* global protocols drawPath */

// eslint-disable-next-line no-unused-vars
function make_query() {
  var source = window.localStorage.getItem('StartPointId');
  var dest = window.localStorage.getItem('StopPointId');

  var honest = confirm('Honest-client protocol? Click cancel for the malicous client protocol!');
  var protocol = honest ? protocols.query_honest_client : protocols.query_malicious_client;

  (function get_one_step (source, dest) {
    if (source === dest) {
      return;
    }

    var promise = protocol(source, dest);
    promise.then(function (next_hop) {
      if (next_hop === 'unreachable') {
        alert('unreachable');
        return;
      }
      if (next_hop == null) {
        alert('error!');
        return;
      }

      drawPath([ source, next_hop ]);
      get_one_step(next_hop, dest);
    });
  }(source, dest));
}
