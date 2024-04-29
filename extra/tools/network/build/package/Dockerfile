# Copyright 2024 Robert Bosch GmbH
#
# SPDX-License-Identifier: Apache-2.0


# Python nuitka Builder
# =====================
FROM python:3.8.12-slim-bullseye as nuitka

ENV PATH="/opt/venv/bin:$PATH"
ADD third_party/cantools /src/third_party/cantools
ADD build/package/gencode.py /src/build/package/gencode.py
ADD build/package/convert.py /src/build/package/convert.py
ADD build/package/mimetype.py /src/build/package/mimetype.py

RUN set -eux; \
    apt-get -y update; \
	apt-get -y upgrade; \
    apt-get -y install --no-install-recommends \
        build-essential \
        gcc \
        git \
        patchelf \
    ; \
    apt-get clean; \
    rm -rf /var/lib/apt/lists/*; \
    \
    python -m venv /opt/venv; \
    pip install --upgrade pip; pip install \
        build \
        canopen \
        canmatrix \
        ldfparser \
        lxml \
        nala \
        nuitka \
        pyyaml \
    ; \
    pip list;

RUN set -eux; \
    mkdir -p /tmp/cantools; \
    tar -xzf /src/third_party/cantools/cantools.tar.gz -C /tmp/cantools --strip-components=1; \
    ls -R /tmp/cantools \
    && \
    (cd /tmp/cantools; python -m build --wheel --outdir dist/ .); \
    pip install /tmp/cantools cantools; \
    python -m nuitka --output-dir=/gencode --standalone --onefile /src/build/package/gencode.py; \
    python -m nuitka --output-dir=/convert --standalone --onefile --include-package=canmatrix /src/build/package/convert.py; \
    python -m nuitka --output-dir=/mimetype --standalone --onefile /src/build/package/mimetype.py; \
    pip list;



# Network Builder
# ===============
FROM golang:bullseye as network
ADD . /src
RUN cd /src && make build



# Network Container Image
# =======================
FROM debian:bullseye-slim
LABEL maintainer="timothy.rule@de.bosch.com";

RUN set -eux; \
    apt-get -y update; \
    apt-get -y upgrade; \
    apt-get -y install --no-install-recommends \
        clang \
    ; \
    apt-get clean; \
    rm -rf /var/lib/apt/lists/*


# Install Generate
# ================
COPY --from=nuitka /gencode/gencode.bin /usr/local/bin/gencode


# Install Convert
# ================
COPY --from=nuitka /convert/convert.bin /usr/local/bin/convert


# Install Mimetype
# ================
COPY --from=nuitka /mimetype/mimetype.bin /usr/local/bin/mimetype


# Install Network
# ================
COPY --from=network /src/bin/network /usr/local/bin/network


# Add License Information (from 3rd party repos)
# =======================
COPY build/stage/licenses /licenses


# Setup the entry conditions
# ==========================
# Run docker with --volume parameter to map the simulation folder into /sim.
WORKDIR /sim
COPY build/package/entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh
ENTRYPOINT ["/entrypoint.sh"]
