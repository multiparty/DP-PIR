const readline = require('readline');
const { DPPIRMode } = require('./constants.js');
const { DPPIRExperiment, ChecklistExperiment, SealPIRExperiment } = require('./experiment.js');

// Create a new command line interface for reading commands from stdin.
function Cmd(orchestrator) {
  this.orchestrator = orchestrator;
  this.rl = readline.createInterface({
    input: process.stdin,
    output: process.stdout,
    terminal: false
  });
}

Cmd.prototype.question = function (question) {
  const self = this;
  return new Promise(function (resolve, reject) {
    self.rl.question(question, resolve);
  });
};

// Prints a help message.
Cmd.prototype.printHelp = function () {
  console.log('Available commands:');
  console.log('- help: prints this help message');
  console.log('- exit: exits the orchestrator');
  console.log('- workers: list all connected workers');
  console.log('- experiments: list all registered experiments');
  console.log('- info <experiment id>: print experiment info');
  console.log('- kill <experiment id>: kills the experiments and resets all its workers');
  console.log('- new <experiment type>: creates a new experiment(s) of the given type');
  console.log('                         possible types are: dppir, checklist, sealpir');
};

// Interactive dialog for creating experiment(s) of a given type.
Cmd.prototype.newDPPIR = async function () {
  // Read arguments.
  const name = await this.question('Give this experiment a unique name: ');  
  const mode = await this.question('Choose mode [online|offline]: ');
  if (mode != DPPIRMode.ONLINE && mode != DPPIRMode.OFFLINE) {
    console.log('ERROR: Invalid mode "' + mode + '"');
    return;
  }
  const tableSize = parseInt(await this.question('Enter the table size: '));
  
  // Read servers' parameters.
  const parties = parseInt(await this.question('Enter number of parties: '));
  const parallelism = parseInt(await this.question('Enter parallelism factor: '));
  if (parties * parallelism > this.orchestrator.serversCount) {
    console.log('ERROR: Not enough servers');
    return;
  }

  const batchSize = parseInt(await this.question('Enter the batch size (per machine): '));
  const span = parseInt(await this.question('Enter the noise distribution span: '));
  const cutoff = parseInt(await this.question('Enter the noise distribution cutoff: '));

  // Read clients' arguments.
  const clients = parseInt(await this.question('Enter number of client per parallel machine: '));
  if (clients * parallelism > this.orchestrator.clientsCount) {
    console.log('ERROR: Not enough clients');
    return;
  }

  const queries = parseInt(await this.question('Enter the number of queries per client: '));
  if (queries * clients != batchSize) {
    console.log('ERROR: Queries and batch are incompatible');
    return;
  }

  // Create experiment.
  const experiment = new DPPIRExperiment(name, mode, clients * parallelism, parties * parallelism);
  experiment.setTableSize(tableSize);
  experiment.setServerParams(parties, parallelism, batchSize, span, cutoff);
  experiment.setClientParams(clients, queries);
  this.orchestrator.addExperiment(experiment);
};
Cmd.prototype.newChecklist = async function () {
  const name = await this.question('Give this experiment a unique name: ');  
  const tableSize = parseInt(await this.question('Enter the table size: '));
  const queries = parseInt(await this.question('Enter the number of queries: '));
  this.orchestrator.addExperiment(new ChecklistExperiment(name, tableSize, queries));  
};
Cmd.prototype.newSealPIR = async function () {
  const name = await this.question('Give this experiment a unique name: ');  
  const tableSize = parseInt(await this.question('Enter the table size: '));
  const queries = parseInt(await this.question('Enter the number of queries: '));
  this.orchestrator.addExperiment(new SealPIRExperiment(name, tableSize, queries));
};

// Start the command line interface.
Cmd.prototype.Listen = function () {
  console.log('Starting orchestrator command line interface');
  this.printHelp();
  process.stdout.write('\n> ');

  // Handle commands!
  const self = this;
  this.rl.on('line', async function (line) {
    console.clear();
    console.log('>', line);
    line = line.trim();
    if (line == 'help') {
      self.printHelp();
    }
    if (line == 'exit') {
      process.exit(0);
    }
    if (line == 'workers') {
      self.orchestrator.listWorkers();
    }
    if (line == 'experiments') {
      self.orchestrator.listExperiments();
    }
    if (line.startsWith('info')) {
      const experimentId = parseInt(line.split(' ')[1]);
      self.orchestrator.experimentInfo(experimentId);
    }
    if (line.startsWith('kill')) {
      const experimentId = parseInt(line.split(' ')[1]);
      self.orchestrator.kill(experimentId);
    }
    if (line.startsWith('new')) {
      const experimentType = line.split(' ')[1];
      switch (experimentType) {
        case 'dppir':
          await self.newDPPIR();
          break;
        case 'checklist':
          await self.newChecklist();
          break;
        case 'sealpir':
          await self.newSealPIR();
          break;
        default:
          console.log('ERROR: Unrecoganizable experiment type');
      }
    }
    process.stdout.write('\n> ');
  });
};

module.exports = Cmd;
