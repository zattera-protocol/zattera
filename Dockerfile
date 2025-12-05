FROM ubuntu:22.04

ARG ZATTERA_STATIC_BUILD=ON
ENV ZATTERA_STATIC_BUILD=${ZATTERA_STATIC_BUILD}

ENV LANG=en_US.UTF-8
ENV DEBIAN_FRONTEND=noninteractive

# Install dependencies in a single layer
RUN apt-get update && \
    apt-get install -y \
        autoconf \
        automake \
        autotools-dev \
        bsdmainutils \
        build-essential \
        ca-certificates \
        cmake \
        curl \
        doxygen \
        fcgiwrap \
        gdb \
        git \
        jq \
        libboost-all-dev \
        libbz2-dev \
        libgflags-dev \
        liblz4-dev \
        libreadline-dev \
        libsnappy-dev \
        libssl-dev \
        libtool \
        libzstd-dev \
        ncurses-dev \
        nginx \
        pkg-config \
        python3-jinja2 \
        python3-pip \
        runit \
        virtualenv \
        wget \
        zlib1g-dev \
    && \
    pip3 install --no-cache-dir gcovr && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

ADD . /usr/local/src/zattera

RUN \
    if [ "$BUILD_STEP" = "1" ] || [ ! "$BUILD_STEP" ] ; then \
    cd /usr/local/src/zattera && \
    mkdir build && \
    cd build && \
    cmake \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_ZATTERA_TESTNET=ON \
        -DLOW_MEMORY_NODE=OFF \
        -DCLEAR_VOTES=ON \
        -DSKIP_BY_TX_ID=ON \
        .. && \
    make -j$(nproc) chain_test test_fixed_string plugin_test && \
    ./tests/chain_test && \
    ./tests/plugin_test && \
    ./programs/utils/test_fixed_string && \
    cd /usr/local/src/zattera && \
    doxygen && \
    PYTHONPATH=programs/build_helpers \
    python3 -m zattera_build_helpers.check_reflect && \
    programs/build_helpers/get_config_check.sh && \
    rm -rf /usr/local/src/zattera/build ; \
    fi

RUN \
    if [ "$BUILD_STEP" = "2" ] || [ ! "$BUILD_STEP" ] ; then \
    cd /usr/local/src/zattera && \
    mkdir build && \
    cd build && \
    cmake \
        -DCMAKE_INSTALL_PREFIX=/usr/local/zatterad-test \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_ZATTERA_TESTNET=ON \
        -DLOW_MEMORY_NODE=OFF \
        -DCLEAR_VOTES=ON \
        -DSKIP_BY_TX_ID=ON \
        -DZATTERA_STATIC_BUILD=${ZATTERA_STATIC_BUILD} \
        .. && \
    make -j$(nproc) chain_test test_fixed_string plugin_test && \
    make install && \
    ./tests/chain_test && \
    ./tests/plugin_test && \
    ./programs/utils/test_fixed_string && \
    cd /usr/local/src/zattera && \
    doxygen && \
    PYTHONPATH=programs/build_helpers \
    python3 -m zattera_build_helpers.check_reflect && \
    programs/build_helpers/get_config_check.sh && \
    rm -rf /usr/local/src/zattera/build ; \
    fi

RUN \
    if [ "$BUILD_STEP" = "1" ] || [ ! "$BUILD_STEP" ] ; then \
    cd /usr/local/src/zattera && \
    mkdir build && \
    cd build && \
    cmake \
        -DCMAKE_BUILD_TYPE=Debug \
        -DENABLE_COVERAGE_TESTING=ON \
        -DBUILD_ZATTERA_TESTNET=ON \
        -DLOW_MEMORY_NODE=OFF \
        -DCLEAR_VOTES=ON \
        -DSKIP_BY_TX_ID=ON \
        -DCHAINBASE_CHECK_LOCKING=OFF \
        .. && \
    make -j$(nproc) chain_test plugin_test && \
    ./tests/chain_test && \
    ./tests/plugin_test && \
    mkdir -p /var/cobertura && \
    gcovr --object-directory="../" --root=../ --xml-pretty --gcov-exclude=".*tests.*" --gcov-exclude=".*fc.*" --gcov-exclude=".*app*" --gcov-exclude=".*net*" --gcov-exclude=".*plugins*" --gcov-exclude=".*schema*" --gcov-exclude=".*time*" --gcov-exclude=".*utilities*" --gcov-exclude=".*wallet*" --gcov-exclude=".*programs*" --output="/var/cobertura/coverage.xml" && \
    cd /usr/local/src/zattera && \
    rm -rf /usr/local/src/zattera/build ; \
    fi

RUN \
    if [ "$BUILD_STEP" = "2" ] || [ ! "$BUILD_STEP" ] ; then \
    cd /usr/local/src/zattera && \
    mkdir build && \
    cd build && \
    cmake \
        -DCMAKE_INSTALL_PREFIX=/usr/local/zatterad-low \
        -DCMAKE_BUILD_TYPE=Release \
        -DLOW_MEMORY_NODE=ON \
        -DCLEAR_VOTES=ON \
        -DSKIP_BY_TX_ID=OFF \
        -DBUILD_ZATTERA_TESTNET=OFF \
        -DZATTERA_STATIC_BUILD=${ZATTERA_STATIC_BUILD} \
        .. \
    && \
    make -j$(nproc) && \
    make install && \
    cd .. && \
    ( /usr/local/zatterad-low/bin/zatterad --version \
      | grep -o '[0-9]*\.[0-9]*\.[0-9]*' \
      && echo '_' \
      && git rev-parse --short HEAD ) \
      | sed -e ':a' -e 'N' -e '$!ba' -e 's/\n//g' \
      > /etc/zatterad-version && \
    cat /etc/zatterad-version && \
    rm -rfv build && \
    mkdir build && \
    cd build && \
    cmake \
        -DCMAKE_INSTALL_PREFIX=/usr/local/zatterad-high \
        -DCMAKE_BUILD_TYPE=Release \
        -DLOW_MEMORY_NODE=OFF \
        -DCLEAR_VOTES=OFF \
        -DSKIP_BY_TX_ID=ON \
        -DBUILD_ZATTERA_TESTNET=OFF \
        -DZATTERA_STATIC_BUILD=${ZATTERA_STATIC_BUILD} \
        .. \
    && \
    make -j$(nproc) && \
    make install && \
    rm -rf /usr/local/src/zattera ; \
    fi

RUN \
    apt-get remove -y \
        automake \
        autotools-dev \
        bsdmainutils \
        build-essential \
        cmake \
        doxygen \
        dpkg-dev \
        libboost-all-dev \
        libc6-dev \
        libexpat1-dev \
        libhwloc-dev \
        libibverbs-dev \
        libicu-dev \
        libltdl-dev \
        libncurses-dev \
        libnuma-dev \
        libopenmpi-dev \
        libreadline-dev \
        libssl-dev \
        libtinfo-dev \
        libtool \
        linux-libc-dev \
        m4 \
        make \
        manpages \
        manpages-dev \
        mpi-default-dev \
        python3-dev \
    && \
    apt-get autoremove -y && \
    rm -rf \
        /var/lib/apt/lists/* \
        /tmp/* \
        /var/tmp/* \
        /var/cache/* \
        /usr/include \
        /usr/local/include

RUN useradd -s /bin/bash -m -d /var/lib/zatterad zatterad

RUN mkdir /var/cache/zatterad && \
    chown zatterad:zatterad -R /var/cache/zatterad

ENV HOME=/var/lib/zatterad
RUN chown zatterad:zatterad -R /var/lib/zatterad

VOLUME ["/var/lib/zatterad"]

# rpc service:
EXPOSE 8090
# p2p service:
EXPOSE 2001

# add seednodes from documentation to image
# ADD docs/seednodes.txt /etc/zatterad/seednodes.txt

# add config files to /etc/zatterad/
RUN mkdir -p /etc/zatterad
ADD configs/witness.config.ini /etc/zatterad/witness.config.ini
ADD configs/fullnode.config.ini /etc/zatterad/fullnode.config.ini
ADD configs/broadcast.config.ini /etc/zatterad/broadcast.config.ini
ADD configs/ahnode.config.ini /etc/zatterad/ahnode.config.ini
ADD configs/testnet.config.ini /etc/zatterad/testnet.config.ini
ADD configs/fastgen.config.ini /etc/zatterad/fastgen.config.ini
ADD configs/test.config.ini /etc/zatterad/test.config.ini

# add runit service templates to /etc/zatterad/runit/
RUN mkdir -p /etc/zatterad/runit
ADD deploy/runit/zatterad.run /etc/zatterad/runit/zatterad.run
RUN chmod +x /etc/zatterad/runit/zatterad.run

# add nginx templates
ADD deploy/nginx/zatterad.nginx.conf /etc/nginx/zatterad.nginx.conf
ADD deploy/nginx/zatterad-proxy.conf.template /etc/nginx/zatterad-proxy.conf.template

# add PaaS startup script and service script
ADD deploy/zatterad-paas-bootstrap.sh /usr/local/bin/zatterad-paas-bootstrap.sh
ADD deploy/zatterad-test-bootstrap.sh /usr/local/bin/zatterad-test-bootstrap.sh
ADD deploy/zatterad-healthcheck.sh /usr/local/bin/zatterad-healthcheck.sh
RUN chmod +x /usr/local/bin/zatterad-paas-bootstrap.sh
RUN chmod +x /usr/local/bin/zatterad-test-bootstrap.sh
RUN chmod +x /usr/local/bin/zatterad-healthcheck.sh

# add PaaS runit templates
ADD deploy/runit/zatterad-paas-monitor.run /etc/zatterad/runit/zatterad-paas-monitor.run
ADD deploy/runit/zatterad-snapshot-uploader.run /etc/zatterad/runit/zatterad-snapshot-uploader.run
RUN chmod +x /etc/zatterad/runit/zatterad-paas-monitor.run
RUN chmod +x /etc/zatterad/runit/zatterad-snapshot-uploader.run

# new entrypoint for all instances
# this enables exitting of the container when the writer node dies
# for PaaS mode (elasticbeanstalk, etc)
# AWS EB Docker requires a non-daemonized entrypoint
ADD deploy/zatterad-entrypoint.sh /usr/local/bin/zatterad-entrypoint.sh
RUN chmod +x /usr/local/bin/zatterad-entrypoint.sh
CMD /usr/local/bin/zatterad-entrypoint.sh
