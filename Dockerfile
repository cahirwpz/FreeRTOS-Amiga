# To build and publish image run following commands:
# > docker build -t cahirwpz/freertos-amiga:latest .
# > docker login
# > docker push cahirwpz/freertos-amiga:latest

FROM debian:buster

WORKDIR /root

ADD http://circleci.com/api/v1/project/cahirwpz/FreeRTOS-Amiga-Toolchain/latest/artifacts/0/FreeRTOS-Amiga-Toolchain.tar.gz \
    FreeRTOS-Amiga-Toolchain.tar.gz
RUN apt-get -q update && apt-get upgrade -y
RUN apt-get install -y --no-install-recommends \
            ctags cscope gcc g++ make tmux \
            python3 python3-setuptools python3-prompt-toolkit \
            python3-pil python3-pip python3-wheel python3-dev
RUN tar -C / -xvzf FreeRTOS-Amiga-Toolchain.tar.gz && rm FreeRTOS-Amiga-Toolchain.tar.gz
RUN pip3 install libtmux
