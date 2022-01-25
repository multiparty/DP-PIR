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
CORES=8
ORCHASTRATOR="http://3.80.35.222:8000"
DAEMON="client"  # Or client

if [[ ! -f "lock" ]]; then
  echo "AWS LOG: Installing dependencies..."

  # Install g++
  apt-get clean
  apt-get update
  apt-get install -y build-essential software-properties-common curl
  apt-get install -y gcc-9 g++-9
  update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-9 60 --slave /usr/bin/g++ g++ /usr/bin/g++-9
  update-alternatives --set gcc "/usr/bin/gcc-9"
  apt-get install -y nodejs npm
  apt-get install -y valgrind

  # Install go
  echo "AWS LOG: Installing go..."
  wget https://golang.org/dl/go1.16.4.linux-amd64.tar.gz
  rm -rf /usr/local/go && tar -C /usr/local -xzf go1.16.4.linux-amd64.tar.gz
  touch /root/.bashrc
  echo "export PATH=\$PATH:/usr/local/go/bin" >> /root/.bashrc
  echo "export PATH=\$PATH:/usr/local/go/bin" >> /home/ubuntu/.bashrc
  export PATH="$PATH:/usr/local/go/bin"

  # Install bazel-3.4.1
  echo "AWS LOG: Installing bazel..."
  echo "deb [arch=amd64] https://storage.googleapis.com/bazel-apt stable jdk1.8" | tee /etc/apt/sources.list.d/bazel.list
  curl https://bazel.build/bazel-release.pub.gpg | apt-key add -
  apt-get update && apt-get install -y bazel-3.4.1
  
  # lock to avoid installing dependencies again.
  touch lock
fi

if [[ ! -d "DP-PIR" ]]; then
  echo "AWS LOG: Cloning repo..."
  # Install our repo and build it
  git clone https://github.com/multiparty/DP-PIR.git
  cd DP-PIR
  git submodule init
  git submodule update
else
  echo "AWS LOG: Updating repo..."
  cd DP-PIR
  git pull origin master
  git submodule update
fi

echo "AWS LOG: compiling main..."
bazel-3.4.1 build //drivacy/... --config=opt

echo "AWS LOG: compiling experiments..."
bazel-3.4.1 build //experiments/... --config=opt

# Run the daemon(s)
pids=()
for i in $(seq 1 $CORES)
do
  echo "AWS LOG: Running daemon $i"
  ./experiments/daemon_${DAEMON}.sh "${ORCHASTRATOR}" &
  pids=( "${pids[@]}" "$!" )
done

for pid in ${pids[@]}; do
  wait $pid
done
