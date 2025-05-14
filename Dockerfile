# -------- STAGE 1: Build --------
FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive
ENV VCPKG_ROOT=/opt/vcpkg
ENV PATH="${VCPKG_ROOT}:${PATH}"
ENV CMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake

# Install build tools
RUN apt-get update && apt-get install -y \
    software-properties-common curl gpg wget lsb-release && \
    wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc | gpg --dearmor -o /usr/share/keyrings/kitware-archive-keyring.gpg && \
    echo "deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ $(lsb_release -cs) main" \
      | tee /etc/apt/sources.list.d/kitware.list > /dev/null && \
    apt-get update && \
    apt-get purge -y cmake && \
    apt-get install -y cmake git g++ zip unzip pkg-config ninja-build

# Install newer GCC (for std::format)
RUN add-apt-repository -y ppa:ubuntu-toolchain-r/test && \
    apt-get update && apt-get install -y gcc-13 g++-13 && \
    update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-13 100 && \
    update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-13 100

# Clone and bootstrap vcpkg
RUN git clone https://github.com/microsoft/vcpkg.git ${VCPKG_ROOT} && \
    ${VCPKG_ROOT}/bootstrap-vcpkg.sh

# Install dependencies
RUN ${VCPKG_ROOT}/vcpkg install \
    curl \
    nlohmann-json \
    drogon

RUN apt-get install -y libgumbo-dev

# Copy project files
WORKDIR /app
COPY . .

# Build
RUN rm -rf build && mkdir build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE} && \
    cmake --build . --config Release

# -------- STAGE 2: Runtime --------
FROM ubuntu:22.04

# Install only the required runtime libs
RUN apt-get update && apt-get install -y \
    libcurl4 \
    libgumbo-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy built binary only
COPY --from=builder /app/build/HouseTracker .

CMD ["./HouseTracker"]
