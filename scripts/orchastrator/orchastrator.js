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
let reportedClientsCount;
let reportedPartiesCount;

// A configuration's run.
let totalParties;
let totalClients;
let clientsPerMachine;
let batch_size;
let queries;
let span;
let cutoff;
let mode;

// Store the table and config file contents.
let table;
let config;

// store the time it took to finish things.
// machine/client id ==> time it took for that machine to finish
let clientTimes;
let partyTimes;

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

function configure(parties, parallelism, _clients, _batch_size,
                   _queries, _span, _cutoff, tablePath, _mode) {
  parties = Number(parties);
  parallelism = Number(parallelism);
  _clients = Number(_clients);
  _batch_size = Number(_batch_size);
  _queries = Number(_queries);
  _span = Number(_span);
  _cutoff = Number(_cutoff);

  // Save configurations.
  ready = false;
  allowClients = false;
  totalParties = parties * parallelism;
  totalClients = parallelism * _clients;
  clientsPerMachine = _clients;
  batch_size = _batch_size;
  queries = _queries;
  span = _span;
  cutoff = _cutoff;
  mode = _mode;

  // Reset maps.
  partyMap = {};
  clientMap = {};
  runningIPs = {};
  runningIPsCount = 0;
  killedCount = 0;
  reportedClientsCount = 0;
  reportedPartiesCount = 0;
  clientTimes = {};
  partyTimes = {};

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
  let command = scriptPath + ' --parties=' + parties;
  command += ' --parallelism=' + parallelism;
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
      console.log('Total killed:', killedCount, '/' ,
                  totalParties + totalClients);
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
  res.send(partyID + ' ' + machineID + ' ' + ' ' + batch_size + ' ' +
           span + ' ' + cutoff + ' ' + mode + '\n');
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
  res.send(machineID + ' ' + clientParallelID + ' ' + queries + ' ' + mode +
           '\n');
});

// Report finishing.
app.get('/doneclient/:machine_id/:client_id/:time', (req, res) => {
  console.log('Client ', req.params.machine_id, '-', req.params.client_id,
              ' finished in ', req.params.time, '!');
  reportedClientsCount++;
  const machineID = Number(req.params.machine_id);
  const clientID = Number(req.params.client_id);
  clientTimes[machine_id] = clientTimes[machine_id] || {};
  clientTimes[machine_id][client_id] = Number(req.params.time);
  if (reportedClientsCount == totalClients) {
    console.log('All clients reported!');
  }
  res.send('\n');
});
app.get('/doneparty/:party_id/:machine_id/:time', (req, res) => {
  console.log('Party ', req.params.party_id, '-', req.params.machine_id,
              ' finished in ', req.params.time, '!');
  reportedPartiesCount++;
  const party_id = Number(req.params.party_id);
  const machine_id = Number(req.params.machine_id);
  partyTimes[party_id] = partyTimes[party_id] || {};
  partyTimes[party_id][machine_id] = Number(req.params.time) / 1000.0;
  if (reportedPartiesCount == totalParties) {
    console.log('All parties reported!');
  }
  res.send('\n');
});

// Show/print times and staticstics
function showTime(type, party_id) {
  let timesObj = type == 'clients' ? clientTimes : partyTimes[party_id];
  if (timesObj == null) {
    return;
  }
  console.log(timesObj);
  // Compute max, min, and avg of times.
  let max = Number.MIN_VALUE;
  let min = Number.MAX_VALUE;
  let avg = 0.0;
  const entries = Object.entries(timesObj);
  for (const [_, t] of entries) {
    max = Math.max(max, t);
    min = Math.min(min, t);
    avg += t;
  }
  console.log('Max:', max);
  console.log('Min:', min);
  console.log('Avg:', avg / entries.length);
  console.log('All times are in seconds!');
  console.log('');
}

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
      console.log('- new <parties> <parallelism> <batch_size> ' +
                  '<clients per machine> <queries per client> <dpspan> ' +
                  '<dpcutoff> <table path> <mode>');
      console.log('- clients');
      console.log('- time <clients|parties> [party_id]
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
      const [parties, parallelism, batch_size, clients, queries, span, cutoff,
             table, mode] = line.split(' ').slice(1);
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
      if (Number(queries) * Number(clients) % batch_size != 0) {
        console.log('Queries and batch are incompatible');
        return;
      }
      if (mode != 'online' && mode != 'offline') {
        console.log('Enter valid mode');
        return;
      }
      configure(parties, parallelism, clients, batch_size, queries, span,
                cutoff, table, mode);
    }
    if (line.startsWith('clients')) {
      allowClients = true;
    }
    if (line.startsWith('time')) {
      const [type, party_id] = line.split(' ').slice(1);
      showTime(type, party_id);
    }
  });
});
