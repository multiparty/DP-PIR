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

# clone repo once!
if [[ ! -d "drivacy" ]]
then
  git clone https://github.com/multiparty/drivacy.git
fi

# update repo.
cd drivacy
git pull origin master
git submodule init
git submodule update

# remove obsolete log files.
rm -rf *.log

if [[ ! -f "lock" ]]
then
  touch "lock"
  sudo apt-get update

  # Dependencies
  sudo apt-get install -y g++
  sudo apt-get install -y build-essential
  sudo apt-get install -y valgrind

  # Bazel
  sudo apt install curl gnupg
  curl -fsSL https://bazel.build/bazel-release.pub.gpg | gpg --dearmor > bazel.gpg
  sudo mv bazel.gpg /etc/apt/trusted.gpg.d/
  echo "deb [arch=amd64] https://storage.googleapis.com/bazel-apt stable jdk1.8" | sudo tee /etc/apt/sources.list.d/bazel.list
  sudo apt-get update && sudo apt-get install -y bazel
fi

# Compile with bazel
bazel build ... --config=opt

# Run daemon
for i in $(seq 1 $CORES)
do
  ./scripts/daemon_client.sh
done
--//
