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
        # valgrind \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copier dans le bon ordre pour maximiser le cache Docker
COPY Makefile .
COPY assets/ assets/ 
COPY include/ include/
COPY src/ src/

RUN make all -j"$(nproc)"

CMD ["./gomoku"]
# CMD ["valgrind", "./gomoku"]
