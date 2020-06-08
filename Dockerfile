# To build and publish image run following commands:
# > docker build -t cahirwpz/freertos-amiga:latest .
# > docker login
# > docker push cahirwpz/freertos-amiga:latest

FROM debian:buster-backports

WORKDIR /root

ADD http://circleci.com/api/v1/project/cahirwpz/FreeRTOS-Amiga-Toolchain/latest/artifacts/0/FreeRTOS-Amiga-Toolchain.tar.xz \
    FreeRTOS-Amiga-Toolchain.tar.xz
ADD https://packagecloud.io/install/repositories/github/git-lfs/script.deb.sh \
    script.deb.sh
RUN apt-get -q update && apt-get upgrade -y
RUN apt-get install -y --no-install-recommends gnupg && \
    bash script.deb.sh && rm script.deb.sh
RUN apt-get install -y --no-install-recommends \
            clang-format ctags cscope gcc g++ git git-lfs make openssh-client \
            socat tmux xz-utils python3 python3-dev python3-setuptools \
            python3-pip python3-pil python3-wheel
RUN tar -C / -xvJf FreeRTOS-Amiga-Toolchain.tar.xz && \
    rm FreeRTOS-Amiga-Toolchain.tar.xz
RUN pip3 install libtmux prompt_toolkit pycodestyle
