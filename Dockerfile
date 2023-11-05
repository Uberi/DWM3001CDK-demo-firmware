# build: docker build -t uberi/qorvo-nrf52833-board .
# run: docker run -it uberi/qorvo-nrf52833-board

FROM ubuntu:22.04

ARG DEBIAN_FRONTEND=noninteractive
RUN apt-get -y update
RUN apt-get install -y build-essential wget unzip

WORKDIR /usr/local

# install nRF5 SDK version 17.1.0 (the same version expected by the Qorvo project)
RUN wget -q https://nsscprodmedia.blob.core.windows.net/prod/software-and-other-downloads/sdks/nrf5/binaries/nrf5_sdk_17.1.0_ddde560.zip && unzip nrf5_sdk_17.1.0_ddde560.zip && rm nrf5_sdk_17.1.0_ddde560.zip

# install SEGGER Embedded Studio version 5.42a (the same version expected by the nRF5 SDK)
# it has an installer that has to be run as root, and has a EULA that needs to be scrolled through
RUN wget -q https://www.segger.com/downloads/embedded-studio/Setup_EmbeddedStudio_ARM_v542a_linux_x64.tar.gz && tar xf Setup_EmbeddedStudio_ARM_v542a_linux_x64.tar.gz --strip-components=1 arm_segger_embedded_studio_542a_linux_x64/install_segger_embedded_studio && rm Setup_EmbeddedStudio_ARM_v542a_linux_x64.tar.gz
RUN apt-get install -y libxrender1 libxext6
RUN yes yes | ./install_segger_embedded_studio --copy-files-to /usr/local/segger_embedded_studio_V5.42a

# install nRF command line tools, including nrfjprog for programming using SEGGER J-Link programmers/debuggers
# TODO: as of 2023-10-30, this requires the workaround of disabling udevadm, since the package is still broken for Docker builds: https://forum.segger.com/index.php/Thread/8953-SOLVED-J-Link-Linux-installer-fails-for-Docker-containers-Error-Failed-to-update/
RUN apt-get install -y libusb-1.0-0 apt-utils udev
RUN wget -q https://nsscprodmedia.blob.core.windows.net/prod/software-and-other-downloads/desktop-software/nrf-command-line-tools/sw/versions-10-x-x/10-23-2/nrf-command-line-tools_10.23.2_amd64.deb && dpkg -i nrf-command-line-tools_10.23.2_amd64.deb
RUN echo '#!/bin/bash\necho not running udevadm "$@"' > /usr/bin/udevadm && chmod +x /usr/bin/udevadm && apt-get install -y /opt/nrf-command-line-tools/share/JLink_Linux_V788j_x86_64.deb --fix-broken

# install J-Link tools, including JLink RTT Logger for viewing debug output from the J-Link transferred via SEGGER RTT
RUN wget -q --post-data accept_license_agreement=accepted https://www.segger.com/downloads/jlink/JLink_Linux_V792n_x86_64.tgz && tar xf JLink_Linux_V792n_x86_64.tgz && rm JLink_Linux_V792n_x86_64.tgz

WORKDIR /project
