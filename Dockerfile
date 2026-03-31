# =============================================================================
# Stage 1 – builder
# Full build environment: compiles SFML from source, then the project.
# =============================================================================
FROM ubuntu:24.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential \
        cmake \
        git \
        ca-certificates \
        # SFML system-level dependencies (X11, GL, input, udev)
        libx11-dev \
        libxrandr-dev \
        libxcursor-dev \
        libxi-dev \
        libudev-dev \
        libgl1-mesa-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# ── Layer cache trick ─────────────────────────────────────────────────────────
# Copy only the Makefile first so that rebuilding after source-code changes
# does NOT re-clone and re-build SFML (the slow step).
COPY Makefile .
RUN make sfml

# ── Build the project ─────────────────────────────────────────────────────────
COPY src/     src/
COPY include/ include/
RUN make all

# =============================================================================
# Stage 2 – runtime
# Minimal image: only the binary + SFML shared libraries.
# Needs X11 forwarding to display the window (see run instructions below).
# =============================================================================
FROM ubuntu:24.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
        # X11 / OpenGL runtime (no -dev headers)
        libx11-6 \
        libxrandr2 \
        libxcursor1 \
        libxi6 \
        libudev1 \
        libgl1 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy SFML shared libraries into the standard library path and refresh cache
COPY --from=builder /app/libs/SFML/build/lib/*.so* /usr/local/lib/
RUN ldconfig

# Copy the compiled binary
COPY --from=builder /app/gomoku .

# Requires DISPLAY to be set at runtime for X11 forwarding:
#
#   docker run --rm \
#     -e DISPLAY=$DISPLAY \
#     -v /tmp/.X11-unix:/tmp/.X11-unix \
#     gomoku
#
CMD ["./gomoku"]
