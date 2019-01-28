# Routing Service Demo

A privacy preserving route recommendation demo.

# Service Protocol

The service consists of three important kind of parties:
* _Server_: serves client side html and javascript, routes messages between different components.
* _Backend-Server_: stores the data for the different route recommendations, and serves requests from frontend servers obliviously.
* _Frontend-Servers_: the interface between clients and backend server. 
 Perform preprocessing with the backend server to obfuscate the recommendation data.
 Jointly obfuscate client queries and serve them to and from the backend server. There should be at least 2 front servers.
* _Clients_: make queries to the front-end and back-end servers.

The protocol consists of two stages: (1) Precomputation. (2) Query.

## Precomputation
This happens once every time the route recommendations are updated:

1. The backend server initially possesses a table of the different route recommendations of the form:
[ 'source:destination', 'next hop (first step in the path from source to destination)']
2. The backend server sends this table to the first frontend server.
3. The first front end server shuffles the table and garbles the entries using a PRF (EC scalar multiplication) with two secret keys, one for each column.
4. The front end server sends the garbled table to the next frontend server, which repeates step  .
5. The last front end server sends the garbled table back to the backend server, which stores it.

## Query
Honest Client Query:

 1. The client has a point on the EC representing some ENCODING(source:destination).
 2. Assuming we have n parties/servers. The client comes up with m1, ..., mn random numbers such that: m1 * ... * mn = src mod P
where P is the order of the Elliptic curve used in the PRF from step 3 of the precomputation stage.
 3. The client sends each mi to the corresponding front end server i, and send P * m1 to the backend server.
 4. Each front end server multiplies received mi with its secret key for the first column, and sends result to backend server.
 5. Backend server multiplies all received masks from frontends with its own secret key, and scalar multiplies the result with m1 * P. The result will be Q = P * m1 * k1 * m2 * k2 * ... * mn * kn = P * k1 * ... * kn.
 6. The backend server returns the garbled hop H associted with Q in the stored garbled table.
 7. A symmetric round is executed, where the backend shares H the same way with the frontends and client, frontends multiply the masks with the inverse of their secret key of the second column, and the client reconstruct the actual hop from all the shares.

Malicious Client Query:

1. The client behaves the same way, but sends m1 * P to the first frontend, and sends a regular mask to the backend.
2. The first frontend multiplies m1 * P by q1 * k1, sends q'1 to the backend, and the resulting point to the next frontend, where q1 * q'1 = 1 mod P.
3. Each frontend server multiplies its own qi * ki with the mask received from the client mi and scalar multiplies it with the point it received from previous frontend server. q'i is sent to the backend, and the result is sent to the next frontend.
4. The last frontend does not need qi and q'i. Instead, it just sends the point to the backend after multiplying it with its key and mask from the client.
5. The server multiplies everything together to get Q, and locate H associated with Q in the table.
6. A symmetric round is executed, where the backend shares Q with the frontend and clients, the last frontend receives the point share, and moves it backward up to the first frontend.
7. Each frontend multiplies the point by the inverse of their key, the mask they received from the backend, and a share of the identity.
8. The client multiplies received shares of identity from frontends, the point it receives from the first frontend, and the mask from the server, to reconstruct the actual hop.

## Replicas and Parallelization
For scalability, each party can use several replica machines as opposed to one. So that more user queries can be handled simultanously, and batch/bulk operations (like preprocessing) can be parallelized. For simplicity of implementation, we assume all parties have the same number of replicas. However, the design of the protocols support variable number of replicas per party.

The pre-processing is parallelized by spliting the table into chunks, and having each replica garble a chunk, and send it to a unique replica owned by the next party. Shuffling cannot be done locally by each replica only, we use a single round decentralized shuffle based on two local shuffles to simulate a global shuffle.

Queries are parallized by having the client randomly choose a set of replicas, with one replica belonging to each party, and sending its query to that set. We choose the set by choosing a single number i between 1 and the number of replicas per party. And then picking replica number i from each party's set of replicas. This gives us the same load balancing guarantees, while simplifying coordination between the different replicas in the query set.

# Installation
To install all the needed dependencies run:
```shell
npm install
```
# Running The Demos on a single machine
You can run the entire service on a single machine, so that each replica is a separate thread with a different port. No shared memory is utilized in this case, and all communications are sent through sockets, so behaviour matches that of when every replica is a separate machine.

First, you must scrape the map data for use by the servers and in the client. The map data is gitignored since it is auto-generated by the scraping script. To scrape run:
```shell
./script/scrape.sh
```

After scraping, you are ready to run. Use the following scripts to run the demo. The first script runs all needed parties. The second scripts signals the parties to perform the inital precomputation:
```shell
./scripts/run.sh <number of parties> <number of replicas>
./scripts/recompute.sh
```

In your browser, go to http://localhost:3000 to use the demo's HTML interface. Then choose the queries by clicking on the source and destination pins on the map.

Alternatively, you can use a command line version of the client to test the demo:
```shell
node test.js <source_number> <destination_number>
```

To stop the servers running in the background:
```shell
./scripts/kill.sh
```

## Changing The Number Of Parties and/or replicas per Party
parties/config/config.json contains the configruation for the protocols. All the parties will read this configuration and use it to perform the protocl. The configuration is also served to the clients by the JIFF server (/server.js). If you use the run script, it will automatically generate and update the config file. You will only need to change the config file manually if you are running each replica seperatly from the command line (e.g. on different machines).

# File/Directory Structure
1. _data/_ contains the JSON file for the routing tables, and the js file for the map data to be displayed in the browser.
2. _parties/_ contains the source code for backend and frontend servers, as well as a configuration file for them.
3. _scripts/_ contains scripts for running and killing the service, performing precomputation, and scraping map data online using geojson.
4. _scrape/_ contains the python script used to create the map of boston, and scrape the routes and locations used in this demo by default.
5. _static/_ contains html and js files for the client interface, and client portion of the query protocols. Also, contains js files for libraries used by the client.
6. _server.js_ used for serving the client HTML and routing messages.
7. _test.js_ command line nodejs code for querying the service.

## Dependencies
All dependencies will be install using npm install. package.json contains a preinstall hook to ensure that non-JS dependencies are also installed.
npm install may require SUDO to install these non-JS dependencies. If you do not have sudo, remove the preinstall hook from package.json, and
install these libraries locally while modifying the correct enviroment variables to ensure this code can see them.

These non-JS dependencies include:
1. libsodium: in particular, an experimental fork of it available at https://github.com/KinanBab/libsodium that allows scalarMultiplication without clamping
2. spatialindex-src-1.8.5: for scraping and handling the geojson information

Both libraries will be downloaded as source, compiled, and installed by lib/dependencies.sh and scrape/dependencies.sh respectively, both of which
run when npm install is called.



