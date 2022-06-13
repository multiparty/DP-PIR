const Status = {
  WORKER_READY: -1,
  WORKER_WORKING: -2,
  WORKER_CLEANUP: -3,
  EXPERIMENT_WAITING: -4,
  EXPERIMENT_WORKING: -5,
  EXPERIMENT_CLEANUP: -6,
  EXPERIMENT_DONE: -7,
  toString: function(s) {
    switch (s) {
      case Status.WORKER_READY: return "ready";
      case Status.WORKER_WORKING: return "working";
      case Status.WORKER_CLEANUP: return "cleaning up";
      case Status.EXPERIMENT_WAITING: return "waiting";
      case Status.EXPERIMENT_WORKING: return "working";
      case Status.EXPERIMENT_CLEANUP: return "cleaning up";
      case Status.EXPERIMENT_DONE: return "done";
      default: return "UNKNOWN " + status;
    }
  }
};

const ExperimentType = {
  DPPIR: 'dppir',
  CHECKLIST: 'checklist',
  SEALPIR: 'sealpir'
};

const WorkerType = {
  SERVER: 'server',
  CLIENT: 'client'
};

const DPPIRMode = {
  ONLINE: 'online',
  OFFLINE: 'offline',
  ALL: 'all'
};

module.exports = {
  Status: Status,
  ExperimentType: ExperimentType,
  WorkerType: WorkerType,
  DPPIRMode: DPPIRMode
}
