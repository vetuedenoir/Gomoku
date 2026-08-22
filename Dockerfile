# Dockerfile.fast - Version simple et rapide
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

# Pas de cache mount - juste l'installation normale
RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential \
        clang \
        cmake \
        libsfml-dev \
        fonts-liberation \
        linux-tools-common \
        linux-tools-generic \
        elfutils \
    && rm -rf /var/lib/apt/lists/* \
    && (command -v perf >/dev/null 2>&1 \
        || ln -sf "$(find /usr/lib/linux-tools -type f -name perf 2>/dev/null | head -1)" /usr/local/bin/perf)

WORKDIR /app

# Copier dans le bon ordre pour maximiser le cache Docker
COPY Makefile .
COPY assets/ assets/ 
COPY include/ include/
COPY src/ src/
COPY bench/ bench/

RUN make all bench -j"$(nproc)"

CMD ["./gomoku"]
# CMD ["valgrind", "./gomoku"]
