# Use the official debian base image
FROM debian:latest

# Set the maintainer label
LABEL description="Dystopia 1.3 Builder"

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

# Open a port, set in comm.c and startup
EXPOSE 8888

RUN cd src && \
    make clean && \
    make && \
    chmod +x startup

VOLUME [    "/dystopia-mud/player", \
            "/dystopia-mud/area", \
            "/dystopia-mud/log", \
            "/dystopia-mud/notes", \
            "/dystopia-mud/txt" \
            "/dystopia-mud/src" \
        ]

ENTRYPOINT ["/bin/bash", "-c", "cd src && make && ./startup.sh && tail -f /dev/null"]