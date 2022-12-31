#!/bin/sh

# Bootstrap environment for building the application.
# Supported OS:
#  - Arch Linux
#  - Ubuntu 22.04.1 LTS
#  - Ubuntu 22.10
#  - MacOS with Homebrew

if [ -f /etc/os-release ]; then
os=$(. /etc/os-release; echo ${PRETTY_NAME})
else
os=$(uname)
fi

case $os in
    "Ubuntu 22.10")
        echo "OS: Ubuntu 22.10"
        apt-get update && apt-get install -y sudo

        # 1. Install the required packages
        sudo apt-get update && sudo apt-get -y upgrade
        sudo apt-get install -y \
            cmake make gcc g++ pkgconf build-essential wget \
            libtls-dev libssl-dev libgtkmm-4.0-dev

        # 2. Create and enter deps directory
        mkdir deps
        cd deps

        # 3. Build and install Boost 1.80
        wget -O boost_1_80_0.tar.gz https://sourceforge.net/projects/boost/files/boost/1.80.0/boost_1_80_0.tar.gz/download
        tar xf boost_1_80_0.tar.gz
        cd boost_1_80_0/
        ./bootstrap.sh --prefix=/usr/local
        sudo ./b2 --with-system --with-json --with-log -j$(nproc) install
        cd ..

        ;;
    "Ubuntu 22.04.1 LTS")
        echo "OS: Ubuntu 22.04.1 LTS"
        apt-get update && apt-get install -y sudo

        # 1. Install the required packages
        sudo apt-get update && sudo apt-get -y upgrade
        sudo apt-get install -y \
            cmake make gcc pkgconf libtls-dev libssl-dev \
            build-essential g++ python-dev-is-python3 autotools-dev libicu-dev libbz2-dev \
            autotools-dev mm-common libgtk-4-bin libgtk-4-common libgtk-4-dev libgtk-4-doc \
            libxml-parser-perl wget
        sudo apt-get reinstall -y docbook-xsl

        # 2. Create and enter deps directory
        mkdir deps
        cd deps

        # 3. Build and install Boost 1.80
        wget -O boost_1_80_0.tar.gz https://sourceforge.net/projects/boost/files/boost/1.80.0/boost_1_80_0.tar.gz/download
        tar xf boost_1_80_0.tar.gz
        cd boost_1_80_0/
        ./bootstrap.sh --prefix=/usr/local
        sudo ./b2 --with-system --with-json --with-log -j$(nproc) install
        cd ..

        # 4. Build and install sigc++ 3.2
        wget https://github.com/libsigcplusplus/libsigcplusplus/releases/download/3.2.0/libsigc++-3.2.0.tar.xz
        tar xf libsigc++-3.2.0.tar.xz
        cd libsigc++-3.2.0/
        ./autogen.sh --prefix=/usr/local
        sudo make -j$(nproc) install
        cd ..

        # 5. Build and install cairomm-1.16
        wget -O cairomm-1.16.tar.gz https://github.com/freedesktop/cairomm/archive/refs/heads/cairomm-1-16.tar.gz
        tar xf cairomm-1.16.tar.gz
        cd cairomm-cairomm-1-16/
        ./autogen.sh --prefix=/usr/local
        sudo make -j$(nproc) install
        cd ..

        # 6. Build and install glibmm-2.68.2
        wget -O glibmm-2.68.2.tar.gz https://github.com/GNOME/glibmm/archive/refs/tags/2.68.2.tar.gz
        tar xzf glibmm-2.68.2.tar.gz
        cd glibmm-2.68.2/
        ./autogen.sh --prefix=/usr/local
        sudo make -j$(nproc) install
        cd ..

        # 7. Build and install pangomm-2.50.1
        wget -O pangomm-2.50.1.tar.gz https://github.com/GNOME/pangomm/archive/refs/tags/2.50.1.tar.gz
        tar xf pangomm-2.50.1.tar.gz
        cd pangomm-2.50.1/
        ./autogen.sh --prefix=/usr/local
        sudo make -j$(nproc) install
        cd ..

        # 8. Build and install GtkMM 4.6.1
        wget -O gtkmm-4.6.1.tar.gz https://github.com/GNOME/gtkmm/archive/refs/tags/4.6.1.tar.gz
        tar xf gtkmm-4.6.1.tar.gz
        cd gtkmm-4.6.1/
        ./autogen.sh --prefix=/usr/local
        sudo make -j$(nproc) install
        cd ..

        ;;
    "Arch Linux")
        echo "OS: Arch Linux"
        pacman -Sy
        pacman -S --noconfirm cmake make gcc pkgconf openssl boost gtkmm-4.0
        ;;
    Darwin)
        echo "OS: Darwin"
        brew update python
        brew link --overwrite python
        brew update
        brew install cmake make gcc pkg-config openssl@3 boost gtkmm4
        ;;
    *)
        echo "OS: ${os} not supported!"
        ;;
esac
