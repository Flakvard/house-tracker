# Use an official Ubuntu base
FROM ubuntu:22.04

# Set up environment
ENV DEBIAN_FRONTEND=noninteractive
ENV VCPKG_ROOT=/opt/vcpkg
ENV PATH="${VCPKG_ROOT}:${PATH}"

# Install system dependencies
RUN apt-get update && apt-get install -y \
    git cmake g++ curl zip unzip pkg-config ninja-build \
    && rm -rf /var/lib/apt/lists/*

# Clone and bootstrap vcpkg
RUN git clone https://github.com/microsoft/vcpkg.git ${VCPKG_ROOT} && \
    ${VCPKG_ROOT}/bootstrap-vcpkg.sh

# Install libraries via vcpkg
RUN ${VCPKG_ROOT}/vcpkg install \
    curl \
    nlohmann-json \
    drogon

# Install gumbo-parser manually
RUN apt-get update && apt-get install -y libgumbo-dev


# Set CMake toolchain environment variable
ENV CMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake

# Create app directory
WORKDIR /app

# Copy only source files (excluding build cache)
COPY . .

# Download the lates cmake and updates in container
RUN apt-get update && \
    apt-get install -y software-properties-common curl gpg && \
    curl -fsSL https://apt.kitware.com/keys/kitware-archive-latest.asc | gpg --dearmor -o /usr/share/keyrings/kitware.gpg && \
    echo "deb [signed-by=/usr/share/keyrings/kitware.gpg] https://apt.kitware.com/ubuntu/ jammy main" > /etc/apt/sources.list.d/kitware.list && \
    apt-get update && \
    apt-get install -y cmake

# Install GCC 13 and set as default
RUN apt-get update && \
    apt-get install -y software-properties-common && \
    add-apt-repository -y ppa:ubuntu-toolchain-r/test && \
    apt-get update && \
    apt-get install -y gcc-13 g++-13 && \
    update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-13 100 && \
    update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-13 100

# Optional: confirm it's working
RUN g++ --version

# Build from scratch inside the container
RUN rm -rf build && mkdir -p build && \
    cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE} && \
    cmake --build . --config Release

# Optional: run the executable (if it's not a web service, or wrap it later)
CMD ["./build/HouseTracker"]
