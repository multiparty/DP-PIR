# DP-PIR

This repository contains a prototype implementation of the DP-PIR protocol accompanying
the Usenix Security 2022 paper "Batched Differentially Private Information Retrieval".

This repository contains our DP-PIR implementation, in addition to code for running
the experiments shown in the paper, including experiments with our two baselines:
[Checklist](https://github.com/dimakogan/checklist) (both Punc and DPF), and
[SealPIR](https://github.com/microsoft/SealPIR).

## Dependencies

We developed and tested this code using ubuntu 20.04, g++-11 and bazel 4.2.1.
node.js is required to run the orchestrator, and python3 is required to run the
plotting scripts. Finally, go 1.16 is required to run Checklist.

We recommend using the provided docker container to obtain all these dependencies
and configuration.

```
# Build the docker container
docker build -t dppir-image .
# Run the docker container in the background
docker run -d -t dppir-image --name dppir
# Open a terminal inside the container to execute commands in
docker exec -it dppir /bin/bash
cd /DPPIR
```

## Running a simple example

Our protocol consists of two or more parties and a client. Each of these entities needs
to be run as its own separate process. In reality, these entities should be deployed on
separate machines.

You can run a simple example locally using the provided configuration file at `config/example.txt`.
This example configuration is for two parties, epsilon 1 and delta 0.1, and a database of size
100.

First, you need to run the two parties, each in their own separate terminals. If you are using
the docker container, you need to open different terminals in the container for each one.
```
# Run the first party
bazel run //DPPIR:main  -- --config=config/example.txt --stage=all --role=party --party_id=0 --server_id=0
# Run the second party (in a different terminal)
bazel run //DPPIR:main  -- --config=config/example.txt --stage=all --role=party --party_id=1 --server_id=0
```

Then, in a third different terminal, you can run the client and provide it with the number
of queries to make (say 100):
```
bazel run //DPPIR:main  -- --config=config/example.txt --stage=all --role=client --server_id=0 --queries=100
```

Due to our bazel setup, the config files must be located under `config/`. The `--stage` argument
specifies whether to run the `online` or `offline` stages (or both if `all` is provided).

You can generate your own configuration file with your own parameters by running. The absolute
file path should be used for the output config file command line argument:
```
bazel run //DPPIR/config:gen_config -- /full/path/to/config/outfile.txt
```

## Running experiments
We recommend using our orchestrator to run experiments. It will take care of
creating the configuration per the experiment parameter, and assigning and tracking
the progress of the workers.

### On your local machine
First, you must run the orchestrator:
```
cd experiments/orchestrator
# you need to run "npm install" if you are not using the docker container.
node main.js
```

Second, in a separate terminal, you must spawn some local workers. You can do that using:
```
./experiments/local.sh <server worker count> <client worker count>
```

Most of our experiments need only 2 server workers and 1 client worker. However,
our horizontal scaling experiment requires up to 16 server workers and 8 client workers,
and our scaling with parties experiment requires up to 5 server workers. If everything is OK,
you should see output messages in the orchestrator indicating that the workers are connected.

Third, you can use the orchestrator interactive CLI to create experiments and run
them via the workers. For example, you can use `load figure1` to run the experiments
required to generate the data points for figure 1 in the paper.

You can similarly use `load` to reproduce all of the other experiments from the paper.
You can find a description for these experiments under `experiments/orchestrator/stored/*.json`.

You can also create your own custom experiment using `new [dppir|checklist|sealpir]`. You
will then need to follow the instructions on the CLI to specify the experiment parameters.

You can check the status of experiments using the `experiments` command. Whenever
an experiment is finished, the CLI will show a message. The output of experiments
can be found under `experiments/orchestrator/outputs/<experiment-name-and-parameters>/*`.

After loading one of the provided experiments, and waiting for the workers to finish processing
all of its contents, you can plot the results of the stored experiments we provide by using our plotting script:
```
cd experiments/plots
python3 plot.py [ui|pdf] <experiment name>
python3 plot.py ui figure1   # Plots the results of figure1 to the screen
python3 plot.py pdf figure2  # Outputs figure2's plot as a pgf file under experiments/plots/figs/
```

### On AWS
We ran our experiments on AWS. We used ubuntu 20.04 r4.xlarge machines for all the workers.
We put all of our workers in the same `cluster` placement group, for optimized networking
performance. Make sure that all the instances have TCP enabled for ports 8000, and all
ports in the 3000 to 5000 range.

First, you will need to spawn an AWS instance for the orchestrator. After spawning it,
you will need to ssh into the instance and run the orchestrator. You may want to use
the docker container, as the orchestrator will invoke our C++ code to generate configuration
files, and thus requires all its dependencies.

After you ran the orchestrator. You will need to spawn the workers. We spawn the parties
and clients separatly.

Take the public IP of the orchestrator instance, and put it in the designated environment
variable at line 22 in `experiments/aws.sh`. Then, use the launch instance dialog on the
AWS web console to beging spawning some instances. Select `ubuntu 20.04` under `Choose AMI`,
and `r4.xlarge` under `Choose Instance Type` if you want to replicate our setup.

Under `3. Configure Instance`, put the desired number of party/server workers in the
first box `Number of instances`. Check the `Add instance to placement group` box,
and create a new placement group with your desired name and the `cluster` startegy.
Finally, copy all the contents of the modified `experiments/aws.sh` script, and paste it
at the very end of the dialog window under `User data` (`as text`). Continue with the dialog
to spawn the instances, but make sure the security group allows TCP communication
for ports in `[3000, 5000)`.

Then, spawn some client workers by modifying line 23 in `experiments/aws.sh` to
be `client`, and repeating the steps above.

The instances will spawn and automatically install the required dependencies, build
the code, and connect to the orchestrator. On the first start, this may take up to an
hour to complete, afterwhich you should see an indication in the orchestrator that the
workers have connected. If you need to stop and then start the instances again, the time needed
to connect will be shorter.

After the workers connected. You can use the orchestrator CLI to create and manage experiments
similar to our discussion above for running on your local machine. You can check the status of the
workers using the `workers` command, and you can see which `workers` are assigned to some experiment
using `info <experiment id>`. To check on the progress of worker that is **actively processing** an experiment,
using `log <worker id>`. **Caution: ** Using the log command on an inactive worker might cause that work to become corrupted.

In certain cases workers may become corrupted, such as when trying to run experiments that exceed the resources on the workers.
You can try to kill such workers by killing their experiments `kill <experiment id>`. If this does not work out, your best bet
is to exist the orchestrator, use the AWS console to reboot **all** the workers, and run the orchestrator again.

## Repository Strucutre

Our implementation code is under `DPPIR/`, with `DPPIR/main.cc` being the entry point.
The code requires a config file be provided as a command line arguments, which
should be located under `config/`.

Our wrappers for running experiments with the baselines are under `experiments/checklist`
and `experiments/sealpir`.

Our experiment orchestrator is located under `experiments/orchestrator`. The paper experiment
descriptions are under `experiments/orchestrator/stored`. The outputs of running experiments via
the orchestrator will appear under `experiments/orchestrator/outputs` and can be plotted by running
our plotting script at `experiments/plots/plot.py`.

Our measurements which we used to plot the figures in our paper can be found under
`experiments/measurements/`.

```
This repository
│   README.md
│   Dockerfile
└───config
└───DPPIR
│   │   main.cc
└───experiments
    │   orchestrator
    │   measurements
```
