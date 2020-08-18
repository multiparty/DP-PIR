// Input and output directories.
// Server side files contain the plain map data, client side files contain the
// same map data but formatted so it can be rendered in the browser.
const CLIENT_INPUT_PATH = './output/client-raw-data.json';
const SERVER_INPUT_PATH = './output/server-raw-data.json';
const CLIENT_OUTPUT_PATH = './output/client-ready-data.js';
const SERVER_OUTPUT_PATH = './output/server-ready-data.json';

const fs = require('fs');
const hash = require('../client/hash.js');

// Client processing: keep the raw points, but surrond the JSON data
// with JS boilerplate to make it includable in the browser.
const clientInput = require(CLIENT_INPUT_PATH);
let clientContent = 'var points = ' + JSON.stringify(clientInput) + ';\n';
clientContent += 'if(typeof exports !== "undefined") { exports = points; }';

fs.writeFile(CLIENT_OUTPUT_PATH, clientContent, function (err) {
  console.log('Client files written, errors = ', err);
});

// Server processing: values must be hashed so that all entries in the table
// are numbers. Furthermore, the hashing must be consistent with that of the
// client side, hence we do it in JS to use the same library.
const file = require(SERVER_INPUT_PATH);
const hashedTable = [];
for (let row of file) {
  const key = row[0];
  const val = row[1];
  hashedTable.push([
    hash(key, clientInput.features.length),
    hash(val, clientInput.features.length)
  ]);
}

fs.writeFile(SERVER_OUTPUT_PATH, JSON.stringify(hashedTable), function (err) {
  console.log('Server files written, errors = ', err);
});
