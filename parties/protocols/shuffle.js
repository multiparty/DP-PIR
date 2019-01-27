// in place very fast shuffle
var local_shuffle = function (array) { // https://stackoverflow.com/questions/2450954/how-to-randomize-shuffle-a-javascript-array
  var r = Math.random;
  var l = array.length;

  var tmp, index;
  while (l) {
    index = r() * (--l + 1);
    index = index | 0;

    tmp = array[l];
    array[l] = array[index];
    array[index] = tmp;
  }
};

module.exports = function (party) {
  party.protocols.shuffle = async function (tag, array) {
    // first shuffle with the replicas of the same party
    // then combine shuffled and send to next party
    local_shuffle(array);

    var owning_party = party.config.owner;
    var replica_ids = party.config.ids[owning_party];

    // special case, this party is the only replica.
    if (replica_ids.length === 1) {
      return array;
    }

    // Many replicas, chunk the locally shuffled array with them.
    party.protocols.chunk.chunk(replica_ids, tag, array);

    // 1st phase complete, begin second phase.
    array = await party.protocols.chunk.combine(replica_ids, tag);
    local_shuffle(array);

    return array;
  };
};