FROM gcc:14.2.0

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y build-essential \
    cmake \
    make \
    gdb \
    valgrind \
    strace \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace