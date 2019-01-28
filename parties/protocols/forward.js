const $ = require('jquery-deferred');

const messages = {};

module.exports = function (party) {
  party.protocols.forward = {};

  // forward, get, and listen('forward') relies on assumption:
  //   all parties have the same number of replicas.
  party.protocols.forward.forward = function (tag, msg, backwards) {
    var replica_ids = party.config.ids[party.config.owner];
    var id = party.jiff.id;

    var next = id + replica_ids.length;
    if (next > party.config.all_parties.length) {
      next = next - party.config.all_parties.length;
    }

    if (backwards === true) {
      next = id - replica_ids.length;
      if (next <= 0) {
        next = next + party.config.all_parties.length;
      }
    }

    // send over the wire.
    msg = { tag: tag, msg: msg };
    party.jiff.emit('forward', [next], JSON.stringify(msg), false);
  };

  party.protocols.forward.get = function (tag) {
    if (messages[tag] == null) {
      messages[tag] = {};
    }

    if (messages[tag].msg != null) {
      var msg = messages[tag].msg;
      delete messages[tag];
      return msg;
    }

    messages[tag].deferred = $.Deferred();
    return messages[tag].deferred.promise();
  };

  party.jiff.listen('forward', function (sender_id, msg) {
    msg = JSON.parse(msg);
    var tag = msg['tag'];
    msg = msg['msg'];

    if (messages[tag] == null) {
      messages[tag] = { msg: msg };
    }

    if (messages[tag].deferred != null) {
      var deferred = messages[tag].deferred;
      delete messages[tag];
      deferred.resolve(msg);
    }
  });

  // Forward to all replicas of next parties (each replica gets a copy of the msg)
  // relies on chunk protocol in an optimized way
  party.protocols.forward.toAll = function (tag, msg) {
    var nextOwner = party.config.owner + 1;
    if (nextOwner > party.config.owner_count) {
      nextOwner = 1;
    }

    // send over the wire.
    msg = { tag: tag, chunk: msg };
    party.jiff.emit('chunk', party.config.ids[nextOwner], JSON.stringify(msg), false);
  };

  party.protocols.forward.getAll = function (tag) {
    var previousOwner = party.config.owner - 1;
    if (previousOwner < 1) {
      previousOwner = party.config.owner_count;
    }

    return party.protocols.chunk.combine(party.config.ids[previousOwner], tag);
  };
};