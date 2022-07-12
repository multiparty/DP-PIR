# Docker image to build pelton incl. all dependencies
# based on Ubuntu 2004
FROM ubuntu:20.04

MAINTAINER Multiparty.org "https://multiparty.org/"

ENV DEBIAN_FRONTEND=noninteractive
ENV LANG C.UTF-8
ENV PATH /usr/local/bin:$PATH

# Dependencies/tools.
RUN apt-get clean && apt-get update
RUN apt-get install -y curl apt-transport-https gnupg software-properties-common

# g++-11
RUN add-apt-repository -y ppa:ubuntu-toolchain-r/test
RUN apt-get install -y gcc-11 g++-11
RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-11 90 \
                        --slave /usr/bin/gcc-ar gcc-ar /usr/bin/gcc-ar-11 \
                        --slave /usr/bin/gcc-nm gcc-nm /usr/bin/gcc-nm-11 \
                        --slave /usr/bin/gcc-ranlib gcc-ranlib /usr/bin/gcc-ranlib-11 \
                        --slave /usr/bin/g++ g++ /usr/bin/g++-11
RUN update-alternatives --set gcc /usr/bin/gcc-11

# nodejs (for orchestrator only)
RUN apt-get install -y nodejs npm

# Install go (for checklist)
RUN apt-get install -y wget
RUN wget https://golang.org/dl/go1.16.4.linux-amd64.tar.gz
RUN rm -rf /usr/local/go && tar -C /usr/local -xzf go1.16.4.linux-amd64.tar.gz
RUN touch /root/.bashrc
RUN echo "export PATH=\$PATH:/usr/local/go/bin" >> /root/.bashrc
RUN export PATH="$PATH:/usr/local/go/bin"

# Install bazel-4.2.1
RUN curl -fsSL https://bazel.build/bazel-release.pub.gpg | gpg --dearmor >bazel-archive-keyring.gpg
RUN mv bazel-archive-keyring.gpg /usr/share/keyrings
RUN echo "deb [arch=amd64 signed-by=/usr/share/keyrings/bazel-archive-keyring.gpg] https://storage.googleapis.com/bazel-apt stable jdk1.8" | tee /etc/apt/sources.list.d/bazel.list
#echo "deb [arch=amd64 trusted=yes] https://storage.googleapis.com/bazel-apt stable jdk1.8" | tee /etc/apt/sources.list.d/bazel.list
RUN apt-get update && apt-get install -y git bazel-4.2.1
RUN update-alternatives --install /usr/bin/bazel bazel /usr/bin/bazel-4.2.1 60
RUN update-alternatives --set bazel "/usr/bin/bazel-4.2.1"

COPY . /DPPIR

# Install plotting and orchestrator dependencies
RUN apt-get install -y python3-pip
RUN cd /DPPIR/experiments/plots && pip3 install -r requirements.txt
RUN cd /DPPIR/experiments/orchestrator && npm install
