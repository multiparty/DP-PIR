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
ORCHASTRATOR="http://44.204.206.222:8000"
DAEMON="server"  # Or client

if [[ ! -f "lock" ]]; then
  echo "AWS LOG: Installing dependencies..."

  # Dependencies/tools.
  apt-get clean
  apt-get update
  apt-get install -y curl apt-transport-https gnupg

  # g++-11
  add-apt-repository -y ppa:ubuntu-toolchain-r/test
  apt-get install -y gcc-11 g++-11
  update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-11 90 \
                         --slave /usr/bin/gcc-ar gcc-ar /usr/bin/gcc-ar-11 \
                         --slave /usr/bin/gcc-nm gcc-nm /usr/bin/gcc-nm-11 \
                         --slave /usr/bin/gcc-ranlib gcc-ranlib /usr/bin/gcc-ranlib-11 \
                         --slave /usr/bin/g++ g++ /usr/bin/g++-11
  update-alternatives --set gcc /usr/bin/gcc-11

  # nodejs (for orchestrator only)
  apt-get install -y nodejs npm

  # Install go (for checklist)
  echo "AWS LOG: Installing go..."
  wget https://golang.org/dl/go1.16.4.linux-amd64.tar.gz
  rm -rf /usr/local/go && tar -C /usr/local -xzf go1.16.4.linux-amd64.tar.gz
  touch /root/.bashrc
  echo "export PATH=\$PATH:/usr/local/go/bin" >> /root/.bashrc
  echo "export PATH=\$PATH:/usr/local/go/bin" >> /home/ubuntu/.bashrc
  export PATH="$PATH:/usr/local/go/bin"

  # Install bazel-4.2.1
  echo "AWS LOG: Installing bazel..."
  curl -fsSL https://bazel.build/bazel-release.pub.gpg | gpg --dearmor >bazel-archive-keyring.gpg
  mv bazel-archive-keyring.gpg /usr/share/keyrings
  echo "deb [arch=amd64 signed-by=/usr/share/keyrings/bazel-archive-keyring.gpg] https://storage.googleapis.com/bazel-apt stable jdk1.8" | tee /etc/apt/sources.list.d/bazel.list
  #echo "deb [arch=amd64 trusted=yes] https://storage.googleapis.com/bazel-apt stable jdk1.8" | tee /etc/apt/sources.list.d/bazel.list
  apt-get update && apt-get install -y bazel-4.2.1
  update-alternatives --install /usr/bin/bazel bazel /usr/bin/bazel-4.2.1 60
  update-alternatives --set bazel "/usr/bin/bazel-4.2.1"

  # lock to avoid installing dependencies again.
  touch lock
fi

if [[ ! -d "DP-PIR" ]]; then
  echo "AWS LOG: Cloning repo..."
  # Install our repo and build it
  git clone https://github.com/multiparty/DP-PIR DP-PIR
  cd DP-PIR
else
  echo "AWS LOG: Updating repo..."
  cd DP-PIR
  git pull origin main
fi

echo "AWS LOG: compiling main..."
bazel build ... --config=opt

echo "AWS LOG: compiling experiments..."
cd experiments/checklist && bazel build ... -c opt && cd -
cd experiments/sealpir && bazel build ... --config=opt && cd -

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
