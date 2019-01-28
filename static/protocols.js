/* global ECMPC pairHashMap elligatorMap unreached config $ */
(function (exports, node) {
  // Making ajax requests
  var POST;
  if(!node) {
    POST = function (url) {
      return $.ajax({ type: 'GET', url: url, crossDomain: true, cache: false });
    };
  } else {
    POST = function (url) {
      var request = require('request');
      return new Promise(function (resolve, reject) {
        request.get({ url: url }, function (error, response, body) {
          if (error != null) resolve(body);
          else reject(error);
        });
      });
    };
  }

  // Dependencies
  var BN, _ECMPC, _pairHashMap, _elligatorMap, _unreached, _config;
  if (node) {
    var clientData = require('../data/client-map.js');
    _pairHashMap = clientData.pairHashMap;
    _elligatorMap = clientData.elligatorMap;
    _unreached = clientData._unreached;

    _ECMPC = require('../parties/helpers/ECMPC.js');
    _config = require('../parties/config/config.json');
  } else {
    _pairHashMap = pairHashMap;
    _elligatorMap = elligatorMap;
    _unreached = unreached;

    _ECMPC = ECMPC;
    _config = config;
  }

  // Configuration and initialization
  var count = 0;
  var uuid = (function () {
    // https://stackoverflow.com/questions/105034/create-guid-uuid-in-javascript
    var d = new Date().getTime(); // Public Domain/MIT
    if (typeof performance !== 'undefined' && typeof performance.now === 'function'){
      d += performance.now(); //use high-precision timer if available
    }
    return 'xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx'.replace(/[xy]/g, function (c) {
      var r = (d + Math.random() * 16) % 16 | 0;
      d = Math.floor(d / 16);
      return (c === 'x' ? r : (r & 0x3 | 0x8)).toString(16);
    });
  }());

  var urls = {};
  for (var j = 0; j < _config.replicas; j++) {
    urls[j] = [];
    for (var i = 0; i < _config.parties; i++) {
      var offset = (i * _config.replicas) + j + 1;
      urls[j].push('http://localhost:' + (_config.base_port + offset));
    }
  }
  var randomCliqueURLs = function () {
    var r = Math.floor(Math.random() * _config.replicas);
    return urls[r];
  };

  // Protocols
  exports.query_honest_client = function (source, dest) {
    // Generate unique tag
    var tag = uuid+'-'+(count++);

    // Hash query to EC Point and share it
    var queryPoint = _pairHashMap[source + ':' + dest];
    var shares = _ECMPC.share(queryPoint, _config.parties);
    shares[0] = JSON.stringify(shares[0]);

    // Send queries
    var promises = [];
    var urls = randomCliqueURLs();
    for (var i = 0; i < urls.length; i++) {
      var url = urls[i] + '/query/honest/' + tag + '/' + shares[i];
      promises.push(POST(url));
    }

    // Receive responses
    return Promise.all(promises).then(function (results) {
      // parse
      for (var k = 0; k < results.length; k++) {
        results[k] = JSON.parse(results[k])['share'];
      }

      // Reconstruct
      var hop = _ECMPC.reconstruct(results);
      if (hop.toString() === _unreached.toString()) {
        return 'unreachable';
      } else {
        return _elligatorMap[JSON.stringify(hop)];
      }
    });
  };
  exports.query_malicious_client = function (source, dest) {
    // Generate unique tag
    var tag = uuid+'-'+(count++);

    // Hash query to EC Point and share it
    var queryPoint = _pairHashMap[source + ':' + dest];
    var shares = _ECMPC.share(queryPoint, _config.parties);
    shares[0] = JSON.stringify(shares[0]);

    // Swap shares: 1st frontend gets point, backend gets scalar
    var tmp = shares[1];
    shares[1] = shares[0];
    shares[0] = tmp;

    // Send queries
    var promises = [];
    var urls = randomCliqueURLs();
    for (var i = 0; i < urls.length; i++) {
      var url = urls[i] + '/query/malicious/' + tag + '/' + shares[i];
      promises.push(POST(url));
    }

    // Receive responses
    return Promise.all(promises).then(function (results) {
      // parse
      for (var k = 0; k < results.length; k++) {
        results[k] = JSON.parse(results[k])['share'];
      }

      // Swap results: 1st frontend gives us back the point, backend gives us scalar
      var tmp = results[1];
      results[1] = results[0];
      results[0] = tmp;

      // Reconstruct
      var hop = _ECMPC.reconstruct(results);
      if (hop.toString() === _unreached.toString()) {
        return 'unreachable';
      } else {
        return _elligatorMap[JSON.stringify(hop)];
      }
    });
  };
}((typeof exports === 'undefined' ? this.protocols = {} : exports), typeof exports !== 'undefined'));