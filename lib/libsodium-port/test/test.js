// require c++ binding module
var wrapper = require('../wrapper.js');

wrapper.ready.then(function () {
  // Find and hash some Point
  var point = wrapper.hashToPoint('hello');
  var pointStr = point.join(',');

  // Find random scalar and its inverse
  var prime = wrapper.prime;

  // Run several tests
  for (var t = 0; t < 1000; t++) {
    var scalar = wrapper.randomScalar();
    var inv = wrapper.BNToBytes(scalar.invm(prime));
    scalar = wrapper.BNToBytes(scalar);

    // First multiplication
    var result = wrapper.scalarMult(point, scalar);
    // Second multiplication
    result = wrapper.scalarMult(result, inv);

    // Should be equal to original point
    var resultStr = point.join(',');
    if (resultStr !== pointStr) {
      console.log('FAIL SCALAR MULT');
      return;
    }
  }

  console.log('success scalarMult');

  // Run several addition / subtraction tests
  for (t = 0; t < 1000; t++) {
    var r1 = Math.random().toString(36).substring(7);
    var r2 = Math.random().toString(36).substring(7);

    var p1 = wrapper.hashToPoint(r1);
    var p2 = wrapper.hashToPoint(r2);

    var res = wrapper.pointAdd(p1, p2);
    res = wrapper.pointSub(res, p2);

    var p1Str = p1.join(',');
    var rsStr = res.join(',');
    if (rsStr !== p1Str) {
      console.log('FAIL POINT ADD/SUB');
      return;
    }
  }

  console.log('success add/sub');
});