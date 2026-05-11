FROM ubuntu:24.04 AS builder

RUN apt-get update && apt-get install -y --no-install-recommends \
    gcc-arm-none-eabi \
    cmake \
    ninja-build \
    binutils-arm-none-eabi \
    libnewlib-arm-none-eabi \
    libstdc++-arm-none-eabi-newlib \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /asn1pp
COPY . .

RUN cmake -B build \
    -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-stm32f407.cmake \
    -DBAREMETAL_DEMO=ON \
    -DCMAKE_BUILD_TYPE=MinSizeRel && \
    cmake --build build -j$(nproc)

RUN mkdir -p /artifacts && \
    cp build/libs/libasn1pp-codec.a /artifacts/ && \
    cp build/libs/libasn1pp-arch.a /artifacts/ || true && \
    if [ -f build/bare-metal-demo.elf ]; then \
        cp build/bare-metal-demo.elf /artifacts/; \
        arm-none-eabi-size -A build/bare-metal-demo.elf > /artifacts/size-report.txt; \
        arm-none-eabi-nm --size-sort --print-size build/bare-metal-demo.elf > /artifacts/symbol-sizes.txt; \
    fi

CMD ["cat", "/artifacts/size-report.txt"]
