Differentially Private Information Retrieval

## Dependencies

1. [Bazel](https://docs.bazel.build/versions/master/install.html): Tested on version 3.4.1.
2. g++: tested on version 9.3.0. The repo requires compilation with c++17.

## Compilation and Running Tests

To compile the code, run
```bash
bazel build ...
```

You can run all the tests via:
```bash
bazel test ...
```

## Running the Code
You need to run at least two server parties and at least one client to execute an entire round of the offline or online protocol.

You can run these instances using `scripts/run_client_offline.sh`, `scripts/run_client_online.sh`, `scripts/run_parties_offline.sh`, and `scripts/run_parties_online.sh`.

Running any of these scripts without command line arguments displays a help usage message.

These scripts require a valid configuration file, containing the ip and port of the various parties, as well as various other configurations, such as the number of parties and their public keys. To generate such a configuration file, compile the code with bazel, then run `./bazel-bin/config <args...> > config.json`. Running this command without any command line arguments displays a help usage message.
