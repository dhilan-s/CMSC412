FROM ubuntu:trusty

COPY sources.list /etc/apt/sources.list
RUN apt-get update
RUN apt-get -y upgrade
RUN apt-get -y install build-essential nasm libc6-dev-i386 git gdb x11-utils dos2unix python ruby
RUN apt-get -y build-dep qemu
RUN apt-get -y install qemu-system-i386
RUN apt-get -y install curl
RUN apt-get -y install software-properties-common
RUN add-apt-repository ppa:openjdk-r/ppa
RUN apt-get -y update
RUN apt-get -y --force-yes install openjdk-8-jdk
