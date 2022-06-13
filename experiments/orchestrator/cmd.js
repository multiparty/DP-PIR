const fs = require('fs');
const path = require('path');
const readline = require('readline');
const { DPPIRMode } = require('./constants.js');
const { DPPIRExperiment, ChecklistExperiment, SealPIRExperiment } = require('./experiment.js');

// Expands array values (e.g. 10,20,30) also works for singular values.
const expand = function (value) {
  return value.split(",").map(v => Number(v));
};

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
  console.log('- log <worker id>: shows the current logs of the worker');
  console.log('- new <experiment type>: creates a new experiment(s) of the given type');
  console.log('                         possible types are: dppir, checklist, sealpir');
  console.log('- clear: clears experiment history');
  console.log('- store <filename>: stores a copy of the description of all experiments ');
  console.log('                    in the history to stored/<filename>.json');
  console.log('- load <filename>: loads stored experiment descriptions from ');
  console.log('                   stored/<filename>.json. Use * to load all files');
  console.log('                   under stored/');
  console.log('- loadi <filename>: same as load, but interactively prompts for');
  console.log('                    each experiment before loading it');
};

// Interactive dialog for creating experiment(s) of a given type.
Cmd.prototype.newDPPIR = async function () {
  // Read arguments.
  const baseName = await this.question('Give this experiment a unique name: ');
  const mode = await this.question('Choose mode [online|offline|all]: ');
  if (mode != DPPIRMode.ONLINE && mode != DPPIRMode.OFFLINE && mode != DPPIRMode.ALL) {
    console.log('ERROR: Invalid mode "' + mode + '"');
    return;
  }
  const tableSizes = expand(await this.question('Enter the table size(s): '));

  // Read servers' parameters.
  const parties = expand(await this.question('Enter number of parties(s): '));
  const parallelisms = expand(await this.question('Enter parallelism factor(s): '));

  const batchSizes = expand(await this.question('Enter the total number of queries: '));
  const epsilons = expand(await this.question('Enter epsilon(s): '));
  const deltas = expand(await this.question('Enter delta(s): '));

  // Create the experiments.
  for (const tableSize of tableSizes) {
    for (const ps of parties) {
      for (const pr of parallelisms) {
        for (const batch of batchSizes) {
          for (const epsilon of epsilons) {
            for (const delta of deltas) {
              if (ps * pr > this.orchestrator.serversCount) {
                console.log('ERROR: Not enough servers');
                return;
              }
              if (pr > this.orchestrator.clientsCount) {
                console.log('ERROR: Not enough clients');
                return;
              }

              // Create experiment.
              const cn = pr;
              const sn = ps * pr;
              const name = baseName + '-' + tableSize + '-' + ps + '-' + pr
                           + '-' + batch + '-' + epsilon + '-' + delta;
              const experiment = new DPPIRExperiment(name, mode, cn, sn);
              experiment.setTableSize(tableSize);
              experiment.setServerParams(ps, pr, epsilon, delta);
              experiment.setClientParams(Math.floor(batch / pr));
              this.orchestrator.addExperiment(experiment);
            }
          }
        }
      }
    }
  }
};
Cmd.prototype.newChecklist = async function () {
  const baseName = await this.question('Give this experiment a unique name: ');
  const tableSizes = expand(await this.question('Enter the table size: '));
  const queries = expand(await this.question('Enter the number of queries: '));
  let pirtype = await this.question('Enter pirtype (Punc|DPF, default Punc): ');
  if (pirtype == '') {
    pirtype = 'Punc';
  }
  if (pirtype != 'Punc' && pirtype != 'DPF') {
    console.log('ERROR: Invalid pirtype');
    return;
  }
  for (const tableSize of tableSizes) {
    for (const qs of queries) {
      const name = baseName + '-' + tableSize + '-' + qs;
      this.orchestrator.addExperiment(new ChecklistExperiment(name, tableSize, qs, pirtype));
    }
  }
};
Cmd.prototype.newSealPIR = async function () {
  const baseName = await this.question('Give this experiment a unique name: ');
  const tableSizes = expand(await this.question('Enter the table size: '));
  const queries = expand(await this.question('Enter the number of queries: '));
  for (const tableSize of tableSizes) {
    for (const qs of queries) {
      const name = baseName + '-' + tableSize + '-' + qs;
      this.orchestrator.addExperiment(new SealPIRExperiment(name, tableSize, qs));
    }
  }
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
    if (line.startsWith('log')) {
      const workerId = parseInt(line.split(' ')[1]);
      const worker = self.orchestrator.getWorkerById(workerId);
      console.log('Waiting for log...');
      await worker.askForLog();
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
    if (line.startsWith('clear')) {
      self.orchestrator.clearExperiments();
    }
    if (line.startsWith('store')) {
      const filename = line.split(' ')[1];
      const fpath = path.join(__dirname, 'stored', filename + '.json');
      const str = JSON.stringify(self.orchestrator.experimentsToJSON(), null, 2);
      try {
        fs.writeFileSync(fpath, str);
        console.log('Wrote to "' + fpath + '" successfully!');
      } catch (exception) {
        console.log('Could not write to file');
        console.log(exception);
      }
    }
    if (line.startsWith('load')) {
      try {
        const filename = line.split(' ')[1];
        const interactive = line.startsWith('loadi');
        let filenames = [];
        if (filename == "*") {
          for (let fname of fs.readdirSync(path.join(__dirname, 'stored'))) {
            if (fname.endsWith('.json')) {
              filenames.push(fname.substring(0, fname.length - ('.json').length));
            }
          }
        } else {
          filenames = [filename];
        }
        // Read all files and load content.
        for (const fname of filenames) {
          const fpath = path.join(__dirname, 'stored', fname + '.json');
          let arr = JSON.parse(fs.readFileSync(fpath));
          for (const obj of arr) {
            let experiment = null;
            switch (obj.experimentType) {
              case 'dppir':
                experiment = DPPIRExperiment.fromJSON(obj);
                break;
              case 'checklist':
                experiment = ChecklistExperiment.fromJSON(obj);
                break;
              case 'sealpir':
                experiment = SealPIRExperiment.fromJSON(obj);
                break;
              default:
                throw 'Unknown experiment type ' + obj.experimentType;
            }
            if (interactive) {
              const answer = await self.question('Load ' + experiment.name_ + '? [Y/n]: ');
              if (answer.toLowerCase() == 'n' || answer.toLowerCase() == 'no') {
                continue;
              }
            }
            self.orchestrator.addExperiment(experiment);
            console.log('Loaded', experiment.name_);
          }
          console.log('Successfully loaded', arr.length, 'experiments!');
        }
      } catch (exception) {
        console.log('Could not load from file');
        console.log(exception);
      }
    }
    process.stdout.write('\n> ');
  });
};

module.exports = Cmd;
