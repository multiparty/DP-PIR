const { Status } = require('./constants.js');

function Worker(ip, workerType) {
  this.ip = ip;
  this.id = Worker.COUNT++;
  this.workerType = workerType;
  this.status = null;
  this.experiment = null;
  this.ping = null;
  Worker.WORKERS.push(this);
  Worker.MAP[this.ip] = this;
  this.ready();
  this.logResolve = null;
}

// Static members.
Worker.COUNT = 0;
Worker.WORKERS = [];
Worker.MAP = {};
Worker.findWorkerByIp = function (ip) {
  return Worker.MAP[ip];
};

// Method members.
Worker.prototype.ready = function () {
  if (this.status != Status.WORKER_READY) {
    console.log("Worker", this.id, "is ready");
  }
  this.experiment = null;
  this.status = Status.WORKER_READY;
  this.ping = Date.now();
};
Worker.prototype.assignExperiment = function (experiment) {
  console.log("Assigned experiment", experiment.id, "to worker", this.id);
  this.ping = Date.now();
  this.status = Status.WORKER_WORKING;
  this.experiment = experiment;
};
Worker.prototype.kill = function () {
  if (this.status == Status.WORKER_WORKING) {
    console.log("Worker", this.id, "marked to kill");
    this.status = Status.WORKER_CLEANUP;
  }
};
Worker.prototype.shouldKill = function () {
  this.ping = Date.now();
  return this.status == Status.WORKER_CLEANUP;
};
Worker.prototype.finished = function () {
  console.log("Experiment", this.experiment.id, "on worker", this.id, "finished");
  this.ready();
};
Worker.prototype.lastPing = function () {
  return ((Date.now() - this.ping) / 1000.0) + "sec";
};
Worker.prototype.short = function () {
  return "Worker " + this.id + ", worker type: " + this.workerType
      + ", status: " + Status.toString(this.status)
      + ", last ping: " + this.lastPing() + ", ip: " + this.ip;
};
Worker.prototype.askForLog = function () {
  const self = this;
  return new Promise(function (resolve) { self.logResolve = resolve; });
};
Worker.prototype.showLog = function (log) {
  console.log(log);
  const tmp = this.logResolve;
  this.logResolve = null;
  tmp();
};
Worker.prototype.shouldLog = function () {
  return this.logResolve != null;
};

module.exports = Worker;
