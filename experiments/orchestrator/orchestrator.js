const fs = require('fs');

const { WorkerType } = require('./constants.js');
const Worker = require('./worker.js');

function Orchestrator() {
  this.clientsCount = 0;
  this.serversCount = 0;

  this.workers = [];
  this.experiments = [];
  this.clientExperimentsQueue = [];
  this.serverExperimentsQueue = [];
}

// Worker management.
Orchestrator.prototype.addServerWorker = function (ip) {
  const worker = new Worker(ip, WorkerType.SERVER);
  this.workers.push(worker);
  this.serversCount++;
  return worker.id;
};
Orchestrator.prototype.addClientWorker = function (ip) {
  const worker = new Worker(ip, WorkerType.CLIENT);
  this.workers.push(worker);
  this.clientsCount++;
  return worker.id;
};
Orchestrator.prototype.listWorkers = function () {
  for (const worker of this.workers) {
    console.log(worker.short());
  }
};
Orchestrator.prototype.getWorkerById = function (id) {
  for (const worker of this.workers) {
    if (worker.id == id) {
      return worker;
    }
  }
  console.log("ERROR: cannot find worker with id", id);
};
Orchestrator.prototype.workerFinished = function (worker, result) {
  const experiment = worker.experiment;
  // Write result to file.
  const filePath = experiment.resultFile(worker);
  fs.writeFileSync(filePath, result);
  // Update status of worker and experiment.
  worker.finished();
  experiment.finished(worker);
};

// Experiment management.
Orchestrator.prototype.addExperiment = function (experiment) {
  this.experiments.push(experiment);
  if (experiment.clientsNum > 0) {
    this.clientExperimentsQueue.push(experiment);
  }
  if (experiment.serversNum > 0) {
    this.serverExperimentsQueue.push(experiment);
  }
};
Orchestrator.prototype.listExperiments = function () {
  for (const experiment of this.experiments) {
    console.log(experiment.short());
  }
};
Orchestrator.prototype.experimentInfo = function (experimentId) {
  for (const experiment of this.experiments) {
    if (experiment.id == experimentId) {
      console.log(experiment.info());
      return;
    }
  }
  console.log("ERROR: no such experiment");
};
Orchestrator.prototype.kill = function (experimentId) {
  for (const experiment of this.experiments) {
    if (experiment.id == experimentId) {
      for (const worker of experiment.clients) {
        worker.kill();
      }
      for (const worker of experiment.servers) {
        worker.kill();
      }
    }
  }
  console.log("ERROR: no such experiment");
};
Orchestrator.prototype.assignExperiment = function (worker) {
  worker.ready();
  if (worker.workerType == WorkerType.CLIENT) {
    if (this.clientExperimentsQueue.length == 0
        || !this.clientExperimentsQueue[0].readyForClients()) {
      return null;
    }
    const experiment = this.clientExperimentsQueue[0];
    experiment.assignWorker(worker);
    if (experiment.clientsNum == experiment.clients.length) {
      this.clientExperimentsQueue.shift();
    }
    return experiment;
  }
  if (worker.workerType == WorkerType.SERVER) {
    if (this.serverExperimentsQueue.length == 0) {
      return null;
    }
    const experiment = this.serverExperimentsQueue[0];
    experiment.assignWorker(worker);
    if (experiment.serversNum == experiment.servers.length) {
      this.serverExperimentsQueue.shift();
    }
    return experiment;
  }
};


module.exports = Orchestrator;
