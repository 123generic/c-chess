FROM debian:bullseye-slim

LABEL maintainer="mehir@ucsb.edu"
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    gdb \
    git \
    manpages-dev

WORKDIR /workspace
CMD [ "bash" ]
