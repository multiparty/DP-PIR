// Chunks a given array into slices, each slice is sent to a different replica.
// Chunks are continuous slices of the given array of equal length, the first
// chunk is sent to the first replica, the second chunk to the second replica
// and so on.

const $ = require('jquery-deferred');

const chunks = {};
const callbacks = {};

const concatAll = function (tag, replicas) {
  var _chunks = chunks[tag];

  if (replicas.length === 1) {
    return _chunks[replicas[0]];
  }

  var array = [];
  for (var i = 0; i < replicas.length; i++) {
    var replica = replicas[i];
    array = array.concat(_chunks[replica]);
  }

  return array;
};

const tryResolve = function (tag) {
  var _callback = callbacks[tag];
  if (_callback == null) {
    return;
  }

  var _chunks = chunks[tag];
  if (_chunks.size === _callback.replicas.length) {
    var combined = concatAll(tag, _callback.replicas);
    var deferred = _callback.deferred;

    delete callbacks[tag];
    delete chunks[tag];

    deferred.resolve(combined);
  }
};

const receive = function (sender_id, tag, chunk) {
  if (chunks[tag] == null) {
    chunks[tag] = { size: 0 };
  }
  chunks[tag][sender_id] = chunk;
  chunks[tag].size++;

  tryResolve(tag);
};

module.exports = function (party) {
  // Call this when this party has an array and must chunk it with
  // others (possibly including self, and possible only a single other)
  party.protocols.chunk = {};
  party.protocols.chunk.chunk = function (replica_ids, tag, array) {
    var chunkCount = replica_ids.length;
    var chunkSize = Math.ceil(array.length / chunkCount);

    for (var i = 0; i < chunkCount; i++) {
      var receiver = replica_ids[i];

      // chunk it!
      var chunk = array;
      if (chunkCount > 1) {
        chunk = array.slice(i * chunkSize, (i+1) * chunkSize);
      }

      // special case, no need to send anything to this party.
      if (receiver === party.jiff.id) {
        receive(receiver, tag, chunk);
        continue;
      }

      // send over the wire.
      var msg = { tag: tag, chunk: chunk };
      party.jiff.emit('chunk', [receiver], JSON.stringify(msg), false);
    }
  };

  // Call this when you are expecting others (potentially including self, and
  // possibly just a single other) to chunk arrays with you.
  party.protocols.chunk.combine = function (replica_ids, tag) {
    if (chunks[tag] == null) {
      chunks[tag] = { size: 0 };
    }

    // everything is ready, concat and return.
    if (chunks[tag].size === replica_ids.length) {
      var result = concatAll(tag, replica_ids);
      delete chunks[tag];
      return result;
    }

    // some chunks are missing, return a promise to them concatenated.
    var deferred = $.Deferred();
    callbacks[tag] = { replicas: replica_ids, deferred: deferred };

    return deferred.promise();
  };

  // Listens to received chunks and set them up to be matched with the correct
  // combine call.
  party.jiff.listen('chunk', function (sender_id, msg) {
    msg = JSON.parse(msg);
    var chunk = msg['chunk'];
    var tag = msg['tag'];

    receive(sender_id, tag, chunk);
  });
};