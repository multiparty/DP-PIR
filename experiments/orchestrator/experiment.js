const { exec } = require('child_process');
const path = require('path');
const fs = require('fs');

const { Status, WorkerType, ExperimentType } = require('./constants.js');

const OUTPUT_PREFIX = path.join(__dirname, 'outputs');

function AbstractExperiment(name, experimentType, clientsNum, serversNum) {
  this.id = AbstractExperiment.COUNT++;
  this.name_ = name;
  this.experimentType = experimentType;
  this.status = Status.EXPERIMENT_WAITING;
  this.startTime = null;
  this.clients = [];
  this.servers = [];
  this.clientsNum = clientsNum;
  this.serversNum = serversNum;
  this.finishedClients = 0;
  this.finishedServers = 0;
  // Create the result diretory.
  fs.mkdirSync(this.resultDirectory(), { recursive: true });
}

// Static members.
AbstractExperiment.COUNT = 0;

// Method members.
AbstractExperiment.prototype.timeElapsed = function () {
  switch (this.status) {
    case Status.EXPERIMENT_WORKING:
      return "working for " + ((Date.now() - this.startTime) / 1000.0) + "sec";
    default:
      return Status.toString(this.status);
  }
};
AbstractExperiment.prototype.assignWorker = function (worker) {
  worker.assignExperiment(this);
  if (worker.workerType == WorkerType.CLIENT) {
    this.clients.push(worker);
  } else if (worker.workerType == WorkerType.SERVER) {
    this.servers.push(worker);
  }
  if (this.clients.length == this.clientsNum && this.servers.length == this.serversNum) {
    console.log("Experiment", this.id, "started!");
    this.status = Status.EXPERIMENT_WORKING;
    this.startTime = Date.now();
  }
};
AbstractExperiment.prototype.finished = function (worker) {
  if (worker.workerType == WorkerType.CLIENT) {
    this.finishedClients++;
    if (this.finishedClients == this.clientsNum) {
      if (this.experimentType == ExperimentType.CHECKLIST) {
        this.status = Status.EXPERIMENT_CLEANUP;
        console.log("Experiment " + this.id + " is cleaning up!");
        for (const server of this.servers) {
          server.kill();
        }
      }
      if (this.serversNum == 0 || this.finishedServers == this.serversNum) {
        this.status = Status.EXPERIMENT_DONE;
        console.log("Experiment " + this.id + " is finished!");
      }
    }
  }
  if (worker.workerType == WorkerType.SERVER) {
    this.finishedServers++;
    if (this.finishedServers == this.serversNum && this.finishedClients == this.clientsNum) {
      this.status = Status.EXPERIMENT_DONE;
      console.log("Experiment " + this.id + " is finished!");
    }
  }
};
AbstractExperiment.prototype.short = function () {
  let str = this.experimentType + " Experiment " + this.id + " (" + this.name_ + ")";
  str += ", status: " + this.timeElapsed();
  return str;
};
AbstractExperiment.prototype.info = function () {
  let str = this.experimentType + " Experiment " + this.id + " (" + this.name_ + "):\n";
  str += "\tstatus: " + this.timeElapsed() + '\n';
  str += "\tclients: [\n\t" + this.clients.map(c => c.id).join("\n\t") + "\n\t]\n";
  str += "\tservers: [\n\t" + this.servers.map(s => s.id).join("\n\t") + "\n\t]\n";
  return str;
};
AbstractExperiment.prototype.resultDirectory = function () {
  return path.join(OUTPUT_PREFIX, this.name_);
};
AbstractExperiment.prototype.readyForClients = function () {
  return true;
};

// DPPIR experiment.
function DPPIRExperiment(name, mode, clientsNum, serversNum) {
  AbstractExperiment.call(this, name, ExperimentType.DPPIR, clientsNum, serversNum);
  this.mode = mode;
  this.clientsMap = {};
  this.serversMap = {};
  this.currentParty = 0;
  this.currentPartyMachine = 0;
  this.currentClientMachine = 0;
  this.currentClientParallel = 0;
  // this.table = null;
  this.config = null;
}
DPPIRExperiment.prototype = Object.create(AbstractExperiment.prototype);
DPPIRExperiment.prototype.setTableSize = function (tableSize) {
  this.tableSize = tableSize;
};
DPPIRExperiment.prototype.setServerParams = function (parties, parallelism, batchSize, span, cutoff) {
  this.parties = parties;
  this.parallelism = parallelism;
  this.batchSize = batchSize;
  this.span = span;
  this.cutoff = cutoff;
};
DPPIRExperiment.prototype.setClientParams = function (clients, queries) {
  this.clientsParallel = clients;
  this.queries = queries;
};
DPPIRExperiment.prototype.generateTableAndConfigurations = async function () {
  if (this.table != null || this.config != null) {
    return;
  }

  // Generate table.
  const self = this;
  /* this.table = await new Promise(function (resolve, reject) {
    const tableScript = path.join(__dirname, '../../scripts/gen_table.py');
    const tableCommand = tableScript + ' ' + self.tableSize;
    exec(tableCommand, (err, stdout, stderr) => {
      resolve(fs.readFileSync(path.join(__dirname, 'table.json'), 'utf8'));
    });
  }); */

  // Generate configuration file!
  this.config = await new Promise(function (resolve, reject) {
    const ipList = self.servers.map(s => s.ip);  
    const scriptPath = path.join(__dirname, '../../bazel-bin/drivacy/config');
    const command = scriptPath + ' --parties=' + self.parties
                               + ' --parallelism=' + self.parallelism
                               + ' ' + ipList.join(' ');
    exec(command, (err, stdout, stderr) => {
      resolve(stdout);
    });
  });
};
// Override.
DPPIRExperiment.prototype.assignWorker = function (worker) {
  if (worker.workerType == WorkerType.CLIENT) {
    this.clientsMap[worker.id] = {
      machine_id: this.currentClientMachine + 1,
      client_id: this.currentClientParallel + 1
    };

    this.currentClientParallel++;
    if (this.currentClientParallel == this.clientsParallel) {
      this.currentClientParallel = 0;
      this.currentClientMachine++;
    }
  } else if (worker.workerType == WorkerType.SERVER) {
    this.serversMap[worker.id] = {
      party_id: this.currentParty + 1,
      machine_id: this.currentPartyMachine + 1
    };

    this.currentPartyMachine++;
    if (this.currentPartyMachine == this.parallelism) {
      this.currentPartyMachine = 0;
      this.currentParty++;
    }
  }
  AbstractExperiment.prototype.assignWorker.call(this, worker);
};
DPPIRExperiment.prototype.resultFile = function (worker) {
  let filename = "";
  if (worker.workerType == WorkerType.CLIENT) {
    filename = "client-" + this.clientsMap[worker.id].machine_id + '-'
               + this.clientsMap[worker.id].client_id;
  }
  if (worker.workerType == WorkerType.SERVER) {
    filename = "party-" + this.serversMap[worker.id].party_id + '-'
               + this.serversMap[worker.id].machine_id;
  }
  return this.resultDirectory() + '/' + filename + '.log';
};
DPPIRExperiment.prototype.info = function () {
  let str = "dppir Experiment " + this.id + " (" + this.name_ + "):\n";
  str += "\tstatus: " + this.timeElapsed() + '\n';
  str += "\tclients:\n";
  for (const worker of this.clients) {
    str += "\t\tworker " + worker.id + " -> "
           + this.clientsMap[worker.id].machine_id + '-'
           + this.clientsMap[worker.id].client_id + '\n';
  }
  str += "\tservers:\n";
  for (const worker of this.servers) {
    str += "\t\tworker " + worker.id + " -> "
           + this.serversMap[worker.id].party_id + '-'
           + this.serversMap[worker.id].machine_id + '\n';
  }
  str += '\tmode: ' + this.mode + '\n';
  str += '\tparties: ' + this.parties + '\n';
  str += '\tparallelism: ' + this.parallelism + '\n';
  str += '\tbatchSize: ' + this.batchSize + '\n';
  str += '\tspan: ' + this.span + '\n';
  str += '\tcutoff: ' + this.cutoff + '\n';
  str += '\tclients: ' + this.clientsParallel + '\n';
  str += '\tqueries: ' + this.queries + '\n';
  str += '\ttableSize: ' + this.tableSize + '\n';
  return str;
};
DPPIRExperiment.prototype.serialize = function (worker) {
  if (worker.workerType == WorkerType.CLIENT) {
    const info = this.clientsMap[worker.id];
    return [this.experimentType, info.machine_id, info.client_id, this.queries, this.mode, this.tableSize].join(' ');
  }
  if (worker.workerType == WorkerType.SERVER) {
    const info = this.serversMap[worker.id];
    return [this.experimentType, info.party_id, info.machine_id, this.batchSize, this.span, this.cutoff, this.mode, this.tableSize].join(' ');
  }
};

// Checklist experiment.
function ChecklistExperiment(name, tableSize, queries) {
  AbstractExperiment.call(this, name, ExperimentType.CHECKLIST, 1, 2);
  this.tableSize = tableSize;
  this.queries = queries;
}
ChecklistExperiment.prototype = Object.create(AbstractExperiment.prototype);
ChecklistExperiment.prototype.resultFile = function (worker) {
  if (worker.workerType == WorkerType.CLIENT) {
    return this.resultDirectory() + '/' + 'client.log';
  } else {
    return this.resultDirectory() + '/' + 'server' + worker.id + '.log';
  }
};
ChecklistExperiment.prototype.serialize = function (worker) {
  if (worker.workerType == WorkerType.CLIENT) {
    const ip1 = this.servers[0].ip + ':' + (10000 + this.servers[0].id);
    const ip2 = this.servers[1].ip + ':' + (10000 + this.servers[1].id);
    return [this.experimentType, 1, 1, ip1, ip2, this.tableSize, this.queries].join(' ');
  }
  if (worker.workerType == WorkerType.SERVER) {
    const port = 10000 + worker.id;
    return [this.experimentType, 1, worker.id, this.tableSize, port].join(' ');
  }
};
ChecklistExperiment.prototype.info = function () {
  let str = AbstractExperiment.prototype.info.call(this);
  str += '\ttableSize: ' + this.tableSize + '\n';
  str += '\tqueries: ' + this.queries + '\n';
  return str;
};
ChecklistExperiment.prototype.readyForClients = function () {
  return this.servers.length == this.serversNum;
};

// SealPIR experiment.
function SealPIRExperiment(name, tableSize, queries) {
  AbstractExperiment.call(this, name, ExperimentType.SEALPIR, 1, 0);
  this.tableSize = tableSize;
  this.queries = queries;
}
SealPIRExperiment.prototype = Object.create(AbstractExperiment.prototype);
SealPIRExperiment.prototype.resultFile = function (worker) {
  return this.resultDirectory() + '/' + 'sealpir.log';
};
SealPIRExperiment.prototype.serialize = function (worker) {
  return [this.experimentType, 1, 1, this.tableSize, this.queries].join(' ');
};
SealPIRExperiment.prototype.info = function () {
  let str = AbstractExperiment.prototype.info.call(this);
  str += '\ttableSize: ' + this.tableSize + '\n';
  str += '\tqueries: ' + this.queries + '\n';
  return str;
};

// Export classes.
module.exports = {
  DPPIRExperiment: DPPIRExperiment,
  ChecklistExperiment: ChecklistExperiment,
  SealPIRExperiment: SealPIRExperiment
};
