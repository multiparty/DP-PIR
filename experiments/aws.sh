Content-Type: multipart/mixed; boundary="//"
MIME-Version: 1.0

--//
Content-Type: text/cloud-config; charset="us-ascii"
MIME-Version: 1.0
Content-Transfer-Encoding: 7bit
Content-Disposition: attachment; filename="cloud-config.txt"

#cloud-config
cloud_final_modules:
- [scripts-user, always]

--//
Content-Type: text/x-shellscript; charset="us-ascii"
MIME-Version: 1.0
Content-Transfer-Encoding: 7bit
Content-Disposition: attachment; filename="userdata.txt"

#!/bin/bash
CORES=1
ORCHASTRATOR="http://18.216.28.28:8000"
DAEMON="client"  # Or server

if [[ ! -f "lock" ]]; then
  # Install g++
  sudo apt-get clean
  sudo apt-get update
  sudo apt-get install -y build-essential software-properties-common curl
  sudo apt-get install -y gcc-9 g++-9
  sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-9 60 --slave /usr/bin/g++ g++ /usr/bin/g++-9
  sudo update-alternatives --set gcc "/usr/bin/gcc-9"
  sudo apt-get install -y nodejs npm
  sudo apt-get install valgrind

  # Install go
  sudo wget https://golang.org/dl/go1.16.4.linux-amd64.tar.gz
  sudo rm -rf /usr/local/go && sudo tar -C /usr/local -xzf go1.16.4.linux-amd64.tar.gz
  sudo echo "export PATH=\$PATH:/usr/local/go/bin" >> /root/.bashrc
  echo "export PATH=\$PATH:/usr/local/go/bin" >> /home/ubuntu/.bashrc

  # Install bazel-3.4.1
  echo "deb [arch=amd64] https://storage.googleapis.com/bazel-apt stable jdk1.8" | sudo tee /etc/apt/sources.list.d/bazel.list
  curl https://bazel.build/bazel-release.pub.gpg | sudo apt-key add -
  sudo apt-get update && sudo apt-get install -y bazel-3.4.1
  
  # lock to avoid installing dependencies again.
  touch lock
fi

if [[ ! -d "DP-PIR" ]]; then
  # Install our repo and build it
  git clone https://github.com/multiparty/DP-PIR.git
  cd DP-PIR
  git submodule init
  git submodule update
  bazel-3.4.1 build //drivacy/... --config=opt
  bazel-3.4.1 build //experiments/... --config=opt
fi

# Run the daemon(s)
pids=()
for i in $(seq 1 $CORES)
do
  ./scripts/daemon_${DAEMON} "${ORCHASTRATOR}" &
  pids=( "${pids[@]}" "$!" )
fi

for pid in ${pids[@]}; do
  wait $pid
done
