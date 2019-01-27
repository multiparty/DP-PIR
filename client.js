/* global mySodiumWrapper BN ECMPC pairHashMap elligatorMap unreached drawPath */

// order of elliptic curve
const prime = mySodiumWrapper.prime;

// query count
var query_count = 0;

// URLS of backend server and then frontend servers
var urls = [ 'http://localhost:9111', 'http://localhost:9114', 'http://localhost:9117' ];

// one step in the query, recursive
function get_one_step(source, dest) {
  var query_number = query_count++;
  var queryPoint = pairHashMap[source + ':' + dest];

  // Share query
  var shares = ECMPC.share(queryPoint, urls.length);
  shares[0] = JSON.stringify(shares[0]);

  // Query all chosen replicas
  var promises = [];
  for (var i = 0; i < urls.length; i++) {
    var url = urls[i]+'/query/honest/'+query_number+'/'+shares[i];
    promises.push($.ajax({ type: 'GET', url: url, crossDomain:true, cache: false }));
  }

  // Retrieve all responses and use them to reconstruct
  Promise.all(promises).then(function (results) {
    for (var i = 0; i < results.length; i++) {
      results[i] = JSON.parse(results[i])['share'];
    }

    var hop = ECMPC.reconstruct(results);
    hop = JSON.stringify(hop);
    var hopPoint = elligatorMap[hop];
    if (hopPoint != null) {
      drawPath([source, hopPoint]);
      if (hopPoint !== dest) {
        get_one_step(hopPoint, dest);
      }
    } else if (hop === unreached) {
      alert('unreachable');
      drawPath([ source, dest ]);
    }
  }).catch(function (error) {
    alert(error);
    drawPath([ source, dest ]);
  });
}

// eslint-disable-next-line no-unused-vars
function make_query() {
  var source = window.localStorage.getItem('StartPointId');
  var dest = window.localStorage.getItem('StopPointId');

  get_one_step(source, dest);
}
