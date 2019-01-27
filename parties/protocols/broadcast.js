const $ = require('jquery-deferred');

const messages = {};

const receive = function (tag, msg) {
  if (messages[tag] == null) {
    messages[tag] = { msg: msg };
  }

  if (messages[tag].deferred != null) {
    var deferred = messages[tag].deferred;
    delete messages[tag];
    deferred.resolve(msg);
  }
};

module.exports = function (party) {
  party.protocols.broadcast = {};

  party.protocols.broadcast.broadcast = function (tag, msg) {
    var replica_ids = party.config.ids[party.config.owner];

    // Double check you are your party's leader
    if (party.jiff.id !== replica_ids[0]) {
      return;
    }

    var _msg = JSON.stringify({ tag: tag, msg: msg });
    party.jiff.emit('broadcast', replica_ids, _msg, false);
  };

  party.protocols.broadcast.get = function (tag) {
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

  party.jiff.listen('broadcast', function (sender_id, msg) {
    if (sender_id !== party.config.ids[party.config.owner][0]) {
      return;
    }

    msg = JSON.parse(msg);
    receive(msg['tag'], msg['msg']);
  });
};