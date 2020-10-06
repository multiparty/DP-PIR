git clone https://github.com/multiparty/drivacy.git
cd drivacy

git checkout experiment
git submodule init
git submodule update

sudo apt-get update

# Dependencies
sudo apt-get install -y g++
sudo apt-get install -y build-essential

# Bazel
sudo apt install curl gnupg
curl -fsSL https://bazel.build/bazel-release.pub.gpg | gpg --dearmor > bazel.gpg
sudo mv bazel.gpg /etc/apt/trusted.gpg.d/
echo "deb [arch=amd64] https://storage.googleapis.com/bazel-apt stable jdk1.8" | sudo tee /etc/apt/sources.list.d/bazel.list
sudo apt-get update && sudo apt-get install -y bazel

# Compile with bazel
bazel build ...
bazel test ...
