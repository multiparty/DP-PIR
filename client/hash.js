// For very big values, we may want to use BigNumber here.
const hash = function (elements, domainSize) {
  let indexInCrossProduct = 0;
  for (let i = 0; i < elements.length; i++) {
    const e = elements[i];
    indexInCrossProduct += Math.pow(domainSize, i) * e;
  }
  return indexInCrossProduct;
}

if (typeof exports !== undefined) {
  module.exports = hash;
}
