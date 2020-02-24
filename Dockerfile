# To build and publish image run following commands:
# > docker build -t cahirwpz/freertos-amiga:latest .
# > docker login
# > docker push cahirwpz/freertos-amiga:latest

FROM debian:buster

WORKDIR /root

ADD http://circleci.com/api/v1/project/cahirwpz/FreeRTOS-Amiga-Toolchain/latest/artifacts/0/FreeRTOS-Amiga-Toolchain.tar.xz \
    FreeRTOS-Amiga-Toolchain.tar.xz
RUN apt-get -q update && apt-get upgrade -y
RUN apt-get install -y --no-install-recommends \
            ctags cscope gcc g++ make tmux xz-utils \
            python3 python3-setuptools python3-prompt-toolkit \
            python3-pip python3-wheel python3-dev
RUN tar -C / -xvJf FreeRTOS-Amiga-Toolchain.tar.xz && \
    rm FreeRTOS-Amiga-Toolchain.tar.xz
RUN pip3 install libtmux
