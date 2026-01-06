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

# Build from game/build directory
RUN cd game/build && \
    make -f Makefile clean && \
    make -f Makefile && \
    chmod +x ../../devops/startup.sh

# Mount points for runtime data
VOLUME [    "/dystopia-mud/gamedata/player", \
            "/dystopia-mud/gamedata/area", \
            "/dystopia-mud/gamedata/log", \
            "/dystopia-mud/gamedata/notes", \
            "/dystopia-mud/gamedata/txt", \
            "/dystopia-mud/game/src" \
        ]

ENTRYPOINT ["/bin/bash", "-c", "cd game/build && make -f Makefile && cd ../../devops && ./startup.sh && tail -f /dev/null"]