# MTD hands-on lab image.
#
# e9patch rewrites x86_64 ELF binaries, so this image is amd64.
# On Apple Silicon it runs under Docker Desktop's Rosetta 2 x86/amd64 emulation
# (enable "Use Rosetta for x86/amd64 emulation" in Docker Desktop settings).
#
# e9patch v1.0.1 is cloned & built at IMAGE BUILD TIME into /opt/e9patch so the
# lab is ready to use immediately. The exercise Makefile picks it up via $E9.
FROM --platform=linux/amd64 ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
        git \
        build-essential \
        g++ \
        make \
        python3 \
        python3-pip \
        curl \
        ca-certificates \
        file \
        binutils \
        vim \
        less \
    && rm -rf /var/lib/apt/lists/*

# pwntools is only needed for the advanced smashme exploit (exercise 2).
RUN pip3 install --no-cache-dir pwntools || \
    echo "WARNING: pwntools install failed; the advanced smashme demo will be unavailable"

# Build e9patch v1.0.1 from source (June 2026 release).
RUN git clone --depth 1 --branch v1.0.1 https://github.com/GJDuck/e9patch /opt/e9patch \
    && cd /opt/e9patch && ./build.sh \
    && test -x /opt/e9patch/e9tool && test -x /opt/e9patch/e9patch

# The exercise Makefile uses $E9 to locate e9patch (default ./e9patch).
ENV E9=/opt/e9patch

WORKDIR /work

# The repo is bind-mounted at /work by docker-compose. Idle by default;
# students `docker compose exec lab ...` into the container.
CMD ["sleep", "infinity"]
