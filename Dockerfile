# Use the official debian base image
FROM debian:latest

# Set the maintainer label
LABEL description="Dystopia 1.4 Builder"

# Set the working directory in the container
WORKDIR /dystopia-mud

# Prevents dialog boxes from appearing during package installations
ENV DEBIAN_FRONTEND=noninteractive

# Update the package repository and install necessary packages
RUN apt-get update && \
    apt-get upgrade -y && \
    apt-get install -y \
    build-essential \
    curl \
    wget \
    vim \
    git \
    libz-dev \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

COPY . .

# Open a port, set in comm.c
EXPOSE 8888

RUN cd src && \
    make clean && \
    make && \
    chmod +x startup

VOLUME ["/dystopia-1.4/vme/storage"]

ENTRYPOINT ["/bin/bash", "-c", "cd src && ./startup.sh && tail -f /dev/null"]