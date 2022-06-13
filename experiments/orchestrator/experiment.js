const { spawnSync } = require('child_process');
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
AbstractExperiment.prototype.toJSON = function () {
  return {
    name_: this.name_,
    experimentType: this.experimentType,
    clientsNum: this.clientsNum,
    serversNum: this.serversNum
  };
};

// DPPIR experiment.
function DPPIRExperiment(name, mode, clientsNum, serversNum) {
  AbstractExperiment.call(this, name, ExperimentType.DPPIR, clientsNum, serversNum);
  this.mode = mode;
  this.clientsMap = {};
  this.serversMap = {};
  this.currentParty = 0;
  this.currentPartyMachine = 0;
  this.currentClient = 0;
  this.config = null;
}
DPPIRExperiment.prototype = Object.create(AbstractExperiment.prototype);
DPPIRExperiment.prototype.setTableSize = function (tableSize) {
  this.tableSize = tableSize;
};
DPPIRExperiment.prototype.setServerParams = function (parties, parallelism, epsilon, delta) {
  this.parties = parties;
  this.parallelism = parallelism;
  this.epsilon = epsilon;
  this.delta = delta;
};
DPPIRExperiment.prototype.setClientParams = function (queries) {
  this.queries = queries;
};
DPPIRExperiment.prototype.generateTableAndConfigurations = function () {
  if (this.config != null) {
    return;
  }

  // Generate configuration file!
  const configfile = "/tmp/config.txt";
  let command = "bazel run --config=opt //DPPIR/config:gen_config -- "
                  + configfile + " "
                  + this.tableSize + " "
                  + this.epsilon + " "
                  + this.delta + " "
                  + this.parties + " "
                  + this.parallelism;
  for (const s of this.servers) {
    command += " " + s.ip;
  }

  // Run the command.
  const result = spawnSync(command, {shell: true});
  if (result.status != 0 || result.error != undefined) {
    console.log("Could not create configuration for with :gen_config");
    return;
  }
  this.config = fs.readFileSync(configfile);
};
// Override.
DPPIRExperiment.prototype.assignWorker = function (worker) {
  if (worker.workerType == WorkerType.CLIENT) {
    this.clientsMap[worker.id] = this.currentClient++;
  } else if (worker.workerType == WorkerType.SERVER) {
    this.serversMap[worker.id] = {
      party_id: this.currentParty,
      machine_id: this.currentPartyMachine
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
    filename = "client-" + this.clientsMap[worker.id];
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
           + this.clientsMap[worker.id] + '\n';
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
  str += '\tepsilon: ' + this.epsilon + '\n';
  str += '\tdelta: ' + this.delta + '\n';
  str += '\tqueries: ' + this.queries + '\n';
  str += '\ttableSize: ' + this.tableSize + '\n';
  return str;
};
DPPIRExperiment.prototype.serialize = function (worker) {
  if (worker.workerType == WorkerType.CLIENT) {
    const client_id = this.clientsMap[worker.id];
    return [this.experimentType, client_id, this.queries, this.mode].join(' ');
  }
  if (worker.workerType == WorkerType.SERVER) {
    const info = this.serversMap[worker.id];
    return [this.experimentType, info.party_id, info.machine_id, this.mode].join(' ');
  }
};
DPPIRExperiment.prototype.toJSON = function () {
  let obj = AbstractExperiment.prototype.toJSON.call(this);
  return Object.assign(obj, {
    mode: this.mode,
    parties: this.parties,
    parallelism: this.parallelism,
    epsilon: this.epsilon,
    delta: this.delta,
    queries: this.queries,
    tableSize: this.tableSize
  });
};
DPPIRExperiment.fromJSON = function (obj) {
  let expr = new DPPIRExperiment(obj.name_, obj.mode, obj.clientsNum, obj.serversNum);
  expr.setTableSize(obj.tableSize);
  expr.setServerParams(obj.parties, obj.parallelism, obj.epsilon, obj.delta);
  expr.setClientParams(obj.queries);
  return expr;
};

// Checklist experiment.
function ChecklistExperiment(name, tableSize, queries, pirtype) {
  AbstractExperiment.call(this, name, ExperimentType.CHECKLIST, 1, 2);
  this.tableSize = tableSize;
  this.queries = queries;
  this.pirtype = pirtype;
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
    return [this.experimentType, ip1, ip2, this.queries, this.pirtype].join(' ');
  }
  if (worker.workerType == WorkerType.SERVER) {
    const port = 10000 + worker.id;
    return [this.experimentType, worker.id, this.tableSize, port, this.pirtype].join(' ');
  }
};
ChecklistExperiment.prototype.info = function () {
  let str = AbstractExperiment.prototype.info.call(this);
  str += '\ttableSize: ' + this.tableSize + '\n';
  str += '\tqueries: ' + this.queries + '\n';
  str += '\tpirtype: ' + this.pirtype + '\n';
  return str;
};
ChecklistExperiment.prototype.readyForClients = function () {
  return this.servers.length == this.serversNum;
};
ChecklistExperiment.prototype.toJSON = function () {
  let obj = AbstractExperiment.prototype.toJSON.call(this);
  return Object.assign(obj, {
    pirtype: this.pirtype,
    queries: this.queries,
    tableSize: this.tableSize
  });
};
ChecklistExperiment.fromJSON = function (obj) {
  return new ChecklistExperiment(obj.name_, obj.tableSize, obj.queries, obj.pirtype);
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
  return [this.experimentType, this.tableSize, this.queries].join(' ');
};
SealPIRExperiment.prototype.info = function () {
  let str = AbstractExperiment.prototype.info.call(this);
  str += '\ttableSize: ' + this.tableSize + '\n';
  str += '\tqueries: ' + this.queries + '\n';
  return str;
};
SealPIRExperiment.prototype.toJSON = function () {
  let obj = AbstractExperiment.prototype.toJSON.call(this);
  return Object.assign(obj, {
    queries: this.queries,
    tableSize: this.tableSize
  });
};
SealPIRExperiment.fromJSON = function (obj) {
  return new SealPIRExperiment(obj.name_, obj.tableSize, obj.queries);
};

// Export classes.
module.exports = {
  DPPIRExperiment: DPPIRExperiment,
  ChecklistExperiment: ChecklistExperiment,
  SealPIRExperiment: SealPIRExperiment
};
