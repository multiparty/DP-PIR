var CLIENT_INPUT_PATH = './output/client-raw-data.json';
var SERVER_INPUT_PATH = './output/server-raw-data.json';
var CLIENT_OUTPUT_PATH = './output/client-ready-data.js';
var SERVER_OUTPUT_PATH = './output/server-ready-data.json';

var fs = require('fs');

var wrapper = require('../lib/libsodium-port/wrapper.js');

wrapper.ready.then(function () {
  try {
    var str, hash, hstr;
    var unreachablePlain = '0';
    var unreachable = Array.from(wrapper.hashToPoint(unreachablePlain));
    var tmpMap = {};

    // Client processing: keep the raw points, add pointers to hashed pairs!
    var clientInput = require(CLIENT_INPUT_PATH);

    var points = [];
    var elligatorMap = {};
    for (var k = 0; k < clientInput.features.length; k++) {
      str = clientInput.features[k].properties.point_id.toString();
      hash = Array.from(wrapper.hashToPoint(str));

      hstr = JSON.stringify(hash);
      if (elligatorMap[hstr] != null) {
        console.log('duplicate in next hop', str, elligatorMap[hstr]);
      }

      elligatorMap[hstr] = str;
      tmpMap[str] = hash;
      points.push(str);
    }

    var pairHashMap = {};
    var revsPairMap = {};
    for (var p1 = 0; p1 < points.length; p1++) {
      for (var p2 = 0; p2 < points.length; p2++) {
        str = points[p1] + ':' + points[p2];
        hash = Array.from(wrapper.hashToPoint(str));
        pairHashMap[str] = hash;

        hstr = JSON.stringify(hash);
        if (revsPairMap[hstr] != null) {
          console.log('duplicate in source/destination pair', str, revsPairMap[hstr]);
        }
        revsPairMap[hstr] = str;
      }
    }

    var clientContent = 'var points = ' + JSON.stringify(clientInput) + ';\n';
    clientContent += 'var pairHashMap = ' + JSON.stringify(pairHashMap) + ';\n';
    clientContent += 'var elligatorMap = ' + JSON.stringify(elligatorMap) + ';\n';
    clientContent += 'var unreached = ' + JSON.stringify(unreachable) + ';\n';
    clientContent += 'if(typeof exports !== "undefined") { exports.points = points; exports.unreached = unreached; exports.pairHashMap = pairHashMap; exports.elligatorMap = elligatorMap; }\n';

    // Server hashing
    var file = require(SERVER_INPUT_PATH);

    var hashed = [];
    for (var i = 0; i < file.length; i++) {
      var row = file[i];
      str = row[0] + ':' + row[1];

      if (pairHashMap[str] == null) {
        console.log('Found pair in server that was not in the client!', str);
        process.exit(1);
      }

      var dst = row[2].toString();
      if (dst === unreachablePlain) {
        console.log('Found an unreachable hop!');
      } else if (tmpMap[dst] == null) {
        console.log('Found jump in server that is not in the client nor is unreachable!', dst);
        process.exit(1);
      }

      hashed[i] = [ pairHashMap[str], tmpMap[dst] ];
    }

    var serverContent = JSON.stringify(hashed, null, 2);

    // write out to files
    fs.writeFile(SERVER_OUTPUT_PATH, serverContent, console.log);
    fs.writeFile(CLIENT_OUTPUT_PATH, clientContent, console.log);

    console.log('Hashing Done: should output 2 nulls if file write is successful!');
  } catch (Err) {
    console.log(Err);
  }
});
