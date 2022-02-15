ARG GCC_VERSION=11.2
FROM gcc:${GCC_VERSION}

RUN apt-get update
#RUN apt-get install -y build-essential

RUN apt-get install -y ninja-build
#RUN apt-get install -y curl
#RUN DEBIAN_FRONTEND=noninteractive apt-get install -y cmake

# RUN apt-get remove --purge cmake
# RUN curl -LO https://github.com/Kitware/CMake/releases/download/v3.21.5/cmake-3.21.5.tar.gz
# RUN tar -zxf cmake-3.21.5.tar.gz
# RUN cd cmake-3.21.5 && ./bootstrap -- -DCMAKE_USE_OPENSSL=OFF && make install 

ARG CMAKE_VERSION=3.21.5
RUN wget https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-Linux-x86_64.sh \
      -q -O /tmp/cmake-install.sh \
      && chmod u+x /tmp/cmake-install.sh \
      && mkdir /usr/bin/cmake \
      && /tmp/cmake-install.sh --skip-license --prefix=/usr/bin/cmake \
      && rm /tmp/cmake-install.sh

RUN apt-get install -y git 
RUN DEBIAN_FRONTEND=noninteractive TZ=America/Los_Angeles apt-get install -y gdb

ENV PATH="/usr/bin/cmake/bin:${PATH}"

CMD [ "/bin/bash" ]
