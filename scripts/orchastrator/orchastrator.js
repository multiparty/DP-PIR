// Dependencies.
const express = require('express');
const readline = require('readline');
const ipaddr = require('ipaddr.js');
const { exec } = require('child_process');
const path = require('path');
const fs = require('fs');

// Configurations.
const PORT = 8000;

// Create app and stdin readers.
const app = express();
const rl = readline.createInterface({
  input: process.stdin,
  output: process.stdout,
  terminal: false
});

/*** GLOBAL STATE ***/
// Should kill all polling machines?
let ready;
let shouldKill = false;
let allowClients = false;

// Store the ids of parties and clients that have signed up already.
const partyIPs = [];
const clientIPs = [];

// Maps ip to list of party ids and machine ids.
let partyMap = {};
let clientMap = {};
let runningIPs;
let runningIPsCount;
let killedCount;
let reportedCount;

// A configuration's run.
let totalParties;
let totalClients;
let clientsPerMachine;
let batch;
let queries;
let span;
let cutoff;

// Store the table and config file contents.
let table;
let config;

// store the time it took to finish things.
// machine/client id ==> time it took for that machine to finish
let times;

/*** HELPER FUNCTIONS (over global state) ***/
function formatIP(ipString) {
  if (ipaddr.IPv4.isValid(ipString)) {
    return ipString;
  } else if (ipaddr.IPv6.isValid(ipString)) {
    var ip = ipaddr.IPv6.parse(ipString);
    if (ip.isIPv4MappedAddress()) {
      return ip.toIPv4Address().toString();
    } else {
      // ipString is IPv6
      throw 'Cannot handle ipv6 ' + ipString;
    }
  } else {
    throw 'Invalid IP! ' + ipString;
  }
}

function configure(parties, parallelism, _clients, _batch, _queries, _span, _cutoff, tablePath) {
  parties = Number(parties);
  parallelism = Number(parallelism);
  _clients = Number(_clients);
  _batch = Number(_batch);
  _queries = Number(_queries);
  _span = Number(_span);
  _cutoff = Number(_cutoff);

  // Save configurations.
  ready = false;
  allowClients = false;
  totalParties = parties * parallelism;
  totalClients = parallelism * _clients;
  clientsPerMachine = _clients;
  batch = _batch;
  queries = _queries;
  span = _span;
  cutoff = _cutoff;

  // Reset maps.
  partyMap = {};
  clientMap = {};
  runningIPs = {};
  runningIPsCount = 0;
  killedCount = 0;
  reportedCount = 0;
  times = {};

  // Configure parties.
  const partyIPsCounts = {};
  for (const ip of partyIPs) {
    partyIPsCounts[ip] = (partyIPsCounts[ip] || 0) + 1;
  }
  let ipList = [];
  let partyID = 1;
  let machineID = 1;
  for (const [ip, count] of Object.entries(partyIPsCounts)) {
    partyMap[ip] = [];
    for (let i = 0; i < count; i++) {
      partyMap[ip].push([partyID, machineID]);
      ipList.push(ip);
      machineID++;
      if (machineID > parallelism) {
        machineID = 1;
        partyID++;
        if (partyID > parties)
          break;
      }
    }
    if (partyID > parties)
      break;
  }

  // Configure clients.
  const clientIPsCounts = {};
  for (const ip of clientIPs) {
    clientIPsCounts[ip] = (clientIPsCounts[ip] || 0) + 1;
  }
  machineID = 1;
  clientParallelID = 1;
  for (const [ip, count] of Object.entries(clientIPsCounts)) {
    clientMap[ip] = [];
    for (let i = 0; i < count; i++) {
      clientMap[ip].push([machineID, clientParallelID]);
      clientParallelID++;
      if (clientParallelID > _clients) {
        clientParallelID = 1;
        machineID++;
        if (machineID > parallelism)
          break;
      }
    }
    if (machineID > parallelism)
      break;
  }
  
  // Read table.
  tablePath = path.join(__dirname, tablePath);
  table = fs.readFileSync(tablePath, 'utf8');

  // Generate configuration file!
  const scriptPath = path.join(__dirname, '../../bazel-bin/drivacy/config');
  let command = scriptPath + ' --parties=' + parties + ' --parallelism=' + parallelism;
  command += ' ' + ipList.join(' ');
  exec(command, (err, stdout, stderr) => {
    config = stdout;
    ready = true;
  });
}

/*** SERVER ROUTES ***/
// Server configuration and table files.
app.get('/table', (req, res) => {
  res.send(table);
});
app.get('/config', (req, res) => {
  res.send(config);
});

// Registering IPs with orchestrator.
app.get('/giveip/party', (req, res) => {
  const ip = formatIP(req.connection.remoteAddress);
  partyIPs.push(ip);
  console.log('Party gave ip', ip);
  console.log('Total party ips', partyIPs.length);
  console.log('');
  res.send('\n');
});
app.get('/giveip/client', (req, res) => {
  const ip = formatIP(req.connection.remoteAddress);
  clientIPs.push(ip);
  console.log('Client gave ip', ip);
  console.log('Total client ips', clientIPs.length);
  console.log('');
  res.send('\n');
});

// Killing polling machines.
app.get('/shouldkill', (req, res) => {
  const ip = formatIP(req.connection.remoteAddress);
  if (shouldKill) {
    if (runningIPs[ip] > 0) {
      runningIPs[ip]--;
      killedCount++;
      console.log('Killed machine!');
      console.log('Total killed:', killedCount, '/' , totalParties, totalClients);
      if (killedCount == totalParties + totalClients) {
        killedCount = 0;
        shouldKill = false;
        console.log('All killed!');
      }
      console.log('');
      res.send('1\n');
      return;
    }
  }
  res.send('0\n');
});

// Signing up to receive ids.
app.get('/signup/party/', (req, res) => {
  const ip = formatIP(req.connection.remoteAddress);
  const list = partyMap[ip] || [];
  if (list.length == 0) {
    res.send('WAIT');
    return;
  }
  // Mark the the target party id and machine id.
  runningIPsCount++;
  runningIPs[ip] = (runningIPs[ip] || 0) + 1;
  const [partyID, machineID] = list.shift();
  // Log.
  console.log('Machine', partyID, '-', machineID, 'is about to start!');
  if (runningIPsCount == totalParties) {
    console.log('All machines connected!');
  }
  console.log('');
  // Reply.
  res.send(partyID + ' ' + machineID + ' ' + batch + ' ' + span + ' ' + cutoff + '\n');
});
app.get('/signup/client', (req, res) => {
  // Wait for all parties to connect first.
  if (!allowClients) {
    res.send('WAIT');
    return;
  }
  const ip = formatIP(req.connection.remoteAddress);
  const list = clientMap[ip] || [];
  if (list.length == 0) {
    res.send('WAIT');
    return;
  }
  // Mark the the target client machine id.
  runningIPsCount++;
  runningIPs[ip] = (runningIPs[ip] || 0) + 1;
  const [machineID, clientParallelID] = list.shift();
  // Log.
  console.log('Client', machineID, '-', clientParallelID, 'is about to start!');
  if (runningIPsCount == totalParties + totalClients) {
    console.log('All clients connected!');
  }
  console.log('');
  // Reply.
  res.send(machineID + ' ' + clientParallelID + ' ' + queries + '\n');
});

// Report finishing.
app.get('/done/:machine_id/:client_id/:time', (req, res) => {
  console.log('Client ', req.params.machine_id, '-', req.params.client_id, ' finished in ', req.params.time, '!');
  reportedCount++;
  const machineID = Number(req.params.machine_id);
  const clientID = Number(req.params.client_id);
  const uniqueID = (machineID - 1) * clientsPerMachine + clientID;
  times[uniqueID] = Number(req.params.time);
  if (reportedCount == totalClients) {
    let max = times[1];
    let min = times[1];
    let avg = times[1];
    for (let i = 2; i <= totalClients; i++) {
      max = Math.max(max, times[i]);
      min = Math.min(min, times[i]);
      avg += times[i];
    }
    console.log('Max:', max);
    console.log('Min:', min);
    console.log('Avg:', avg / totalClients);
    console.log('');
  }
  res.send('\n');
});

/*** CLI interfae ***/
app.listen(PORT, () => {
  console.log(`Orchastrator listening at http://localhost:${PORT}`);
  // Start the command line interface.
  // Handle commands!
  rl.on('line', function (line) {
    line = line.trim();
    if (line.startsWith('help')) {
      console.log('Available commands:');
      console.log('- exit');
      console.log('- kill');
      console.log('- new <parties> <parallelism> <clients per machine> <batch> <queries per client> <dpspan> <dpcutoff> <table path>');
      console.log('- clients');
    }
    if (line.startsWith('exit')) {
      process.exit(0);
      return;
    }
    if (line.startsWith('kill')) {
      shouldKill = true;
      return;
    }
    if (line.startsWith('new')) {
      const [parties, parallelism, clients, batch, queries, span, cutoff, table] =
          line.split(' ').slice(1);
      if (shouldKill) {
        console.log('Kill is pending!');
        return;
      }
      if (ready === false) {
        console.log('Another operation is pending!');
        return;
      }
      if (parties * parallelism > partyIPs.length) {
        console.log('Not enough parties');
        return;
      }
      if (Number(parallelism) * Number(clients) > clientIPs.length) {
        console.log('Not enough clients');
        return;
      }
      if (Number(queries) * Number(clients) % batch != 0) {
        console.log('Queries and batch are incompatible');
        return;
      }
      configure(parties, parallelism, clients, batch, queries, span, cutoff, table);
    }
    if (line.startsWith('clients')) {
      allowClients = true;
    }
  });
});
