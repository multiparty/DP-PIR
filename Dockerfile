# Docker image to build DP-PIR with its dependencies as well as
# Checklist and SEALPIR.
FROM ubuntu:groovy-20210225

ENV DEBIAN_FRONTEND=noninteractive
ENV LANG C.UTF-8
ENV PATH /usr/local/bin:$PATH

# Install dependencies
RUN apt-get update && apt-get upgrade -y && apt-get install -y \
    apt-utils git build-essential vim curl software-properties-common \
    gcc-9 g++-9 wget apt-transport-https curl gnupg cmake nodejs npm

RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-9 90 \
                                 --slave /usr/bin/gcc-ar gcc-ar /usr/bin/gcc-ar-9 \
                                 --slave /usr/bin/gcc-nm gcc-nm /usr/bin/gcc-nm-9 \
                                 --slave /usr/bin/gcc-ranlib gcc-ranlib /usr/bin/gcc-ranlib-9 \
                                 --slave /usr/bin/g++ g++ /usr/bin/g++-9
RUN update-alternatives --set gcc /usr/bin/gcc-9

# Bazel repository
RUN echo "deb [arch=amd64] https://storage.googleapis.com/bazel-apt stable jdk1.8" | tee /etc/apt/sources.list.d/bazel.list
RUN curl https://bazel.build/bazel-release.pub.gpg | apt-key add -

# Install bazel
RUN apt-get update
RUN apt-get install -y bazel-3.4.1
RUN cp /usr/bin/bazel-3.4.1 /usr/bin/bazel

# Install go
RUN wget https://golang.org/dl/go1.16.4.linux-amd64.tar.gz
RUN rm -rf /usr/local/go && tar -C /usr/local -xzf go1.16.4.linux-amd64.tar.gz
RUN echo "export PATH=\$PATH:/usr/local/go/bin" >> /root/.bashrc

# Drivacy
WORKDIR /repos
RUN git clone https://github.com/multiparty/DP-PIR.git
WORKDIR DP-PIR
RUN git submodule init
RUN git submodule update
RUN bazel-3.4.1 build ... --config=opt

# ENTRYPOINT ["/bin/bash"]
