# Dictionary

[![Build](https://github.com/grobx/udacity-CppND-final-project/actions/workflows/build.yml/badge.svg)](https://github.com/grobx/udacity-CppND-final-project/actions/workflows/build.yml)

This is my sumbission for Capstone project in the [Udacity C++ Nanodegree Program](https://www.udacity.com/course/c-plus-plus-nanodegree--nd213).

## Description

The application is a simple Dictionary that uses the JSON API from [Merriam-Webster online dictionary](https://www.dictionaryapi.com/) to retrieve the definitions of words and display them.

## How to use it

Start the application, write the word you want to find in the header bar search entry, press enter and wait the response from the Merrian-Webster online service that will appear in the central widget of the application window.

## Organization

### include/app.hpp

This file define the namespace `app` with the following classes:

- Window <- Gtk::ApplicationWindow
- Layout <- Gtk::Box
- Search <- Gtk::SearchEntry
- ResultView <- Gtk::Label

And the following output stream operators:

- std::ostream& operator<< (std::ostream& out, dict::result const& r)
- std::ostream& operator<< (std::ostream& out, dict::entry const& e) 
- std::ostream& operator<< (std::ostream& out, dict::def const& d)
- std::ostream& operator<< (std::ostream& out, dict::sense const& s)

The classes are used to create the user interface, while the stream operators are the one that produces the "pango markup" used to render the `ResultView`.

#### class Window : public Gtk::ApplicationWindow

##### Window (Glib::ustring title, int width, int height)

The `Window` is constructed by using passing a `title`, a `width` and an `height`. During the constructor, an instance of the `Layout` is used as a child widget and a signal for `Gtk::Window::close-request` is attached to the `app::Window::shutdown`.

##### bool shutdown ()

The `shutdown` member function ask the `Gio::Application` to `quit` so that everything is correctly closed and then return `true` to signal the `Window` that it can close now.

#### class Layout : public Gtk::Box

The `Layout` is a vertical `Gtk::Box` that contains an `Gtk::HeaderBar` on the head, an expanded `Gtk::Stack` with a `Gtk::ScrolledWindow` that forms the central widget where the `ResultView` will be rendered, and a `Gtk::Statusbar` on the bottom.

##### Layout ()

You don't need to pass anything to construct a `Layout`. During construction, it setup a `dict::api` class by passing the content of the `DICTIONARY_API_KEY` environment variable then setup the header, central and bottom widgets and finally it initialize the signals by calling `init_signals`.

##### void init_signals ()

The `init_signals` member function is used to setup all the signal handlers that are needed for the application to function as expected. In particular, there are three signals that we are using. The `Layout::realize` signal is attached to a lambda function that will render the message "Application Started!" for 2.5 seconds before deleting it. The `Search::search-changed` is attached to a lambda function that extract the `term` using `Search::get_text` and call `define(term`. The `Search::term-selected` signal is attached to a lambda function that receive the `term` as a parameter and call `define(term)`.

##### void define(Glib::ustring const& term)

The `define` member funciton try to request 

#### class Search : public Gtk::SerachEntry
#### class ResultView : public Gtk::Label

### include/dict.hpp

This file define the namespace `dict` with the following classes:

- api
- result
- suggestions
- entry
- def
- sense

The `api` class is used to send requests to the Merrian-Webster online service. You can construct an instance of this class by calling `api (std::string api_key)`.

This class contains a single member function `result request (std::string word)` that given a `word` either return a `result` object or throws a `suggestions` exception.

You can construct one object of the other classes (`result`, `suggestions`, `entry`, `def` and `sens`) by passing a `json::value` by reference that contains a valid structure for that object.

### include/json_body.hpp

This file is taken from Boost json library example and is used to get the body of the HTTP response as JSON data. It contains the class `json_body` that is made by a `writer` and a `reader`. Only the `reader` is used by this application.

## Dependencies for Building

* cmake >= 3.22
* make >= 4.3
* gcc/g++ >= 11.3
* PkgConfig
* OpenSSL >= 3.0
* Boost >= 1.80
* Gtkmm >= 4.6

Follow the instructions for your OS below in order to fulfill dependencies for building.


### Ubuntu Linux 22.10

1. Install the required packages:
```sh
sudo apt-get update && sudo apt-get -y upgrade
sudo apt-get install -y \
    cmake make gcc g++ pkgconf build-essential wget \
    libtls-dev libssl-dev libgtkmm-4.0-dev
```

2. Create and enter `deps` directory:
```sh
mkdir deps
cd deps
```

3. Build and install Boost 1.80:
```sh
wget -O boost_1_80_0.tar.gz https://sourceforge.net/projects/boost/files/boost/1.80.0/boost_1_80_0.tar.gz/download
tar xf boost_1_80_0.tar.gz
cd boost_1_80_0/
./bootstrap.sh --prefix=/usr/local
sudo ./b2 --with-system --with-json -j$(nproc) install
cd ..
```

### Ubuntu Linux 22.04.1 LTS

1. Install the required packages:
```sh
sudo apt-get update && sudo apt-get -y upgrade
sudo apt-get install -y \
    cmake make gcc pkgconf libtls-dev libssl-dev \
    build-essential g++ python-dev-is-python3 autotools-dev libicu-dev libbz2-dev \
    autotools-dev mm-common libgtk-4-bin libgtk-4-common libgtk-4-dev libgtk-4-doc \
    libxml-parser-perl wget
sudo apt-get reinstall -y docbook-xsl
```

2. Create and enter `deps` directory:
```sh
mkdir deps
cd deps
```

3. Build and install Boost 1.80:
```sh
wget -O boost_1_80_0.tar.gz https://sourceforge.net/projects/boost/files/boost/1.80.0/boost_1_80_0.tar.gz/download
tar xf boost_1_80_0.tar.gz
cd boost_1_80_0/
./bootstrap.sh --prefix=/usr/local
sudo ./b2 --with=all -j$(nproc) install
cd ..
```

4. Build and install sigc++ 3.2:
```sh
wget https://github.com/libsigcplusplus/libsigcplusplus/releases/download/3.2.0/libsigc++-3.2.0.tar.xz
tar xf libsigc++-3.2.0.tar.xz
cd libsigc++-3.2.0/
./autogen.sh --prefix=/usr/local
sudo make -j$(nproc) install
cd ..
```

5. Build and install cairomm-1.16
```sh
wget -O cairomm-1.16.tar.gz https://github.com/freedesktop/cairomm/archive/refs/heads/cairomm-1-16.tar.gz
tar xf cairomm-1.16.tar.gz
cd cairomm-cairomm-1-16/
./autogen.sh --prefix=/usr/local
sudo make -j$(nproc) install
cd ..
```

6. Build and install glibmm-2.68.2
```sh
wget -O glibmm-2.68.2.tar.gz https://github.com/GNOME/glibmm/archive/refs/tags/2.68.2.tar.gz
tar xzf glibmm-2.68.2.tar.gz
cd glibmm-2.68.2/
./autogen.sh --prefix=/usr/local
sudo make -j$(nproc) install
cd ..
```

7. Build and install pangomm-2.50.1
```sh
wget -O pangomm-2.50.1.tar.gz https://github.com/GNOME/pangomm/archive/refs/tags/2.50.1.tar.gz
tar xf pangomm-2.50.1.tar.gz
cd pangomm-2.50.1/
./autogen.sh --prefix=/usr/local
sudo make -j$(nproc) install
cd ..
```

8. Build and install GtkMM 4.6.1
```sh
wget -O gtkmm-4.6.1.tar.gz https://github.com/GNOME/gtkmm/archive/refs/tags/4.6.1.tar.gz
tar xf gtkmm-4.6.1.tar.gz
cd gtkmm-4.6.1/
./autogen.sh --prefix=/usr/local
sudo make -j$(nproc) install
cd ..
```

### Arch Linux

1. Install all the development requirements:

```sh
sudo pacman -S cmake make gcc pkgconf openssl boost gtkmm-4.0
```

2. Export variables:

```sh
export PKG_CONFIG_PATH="/usr/lib/libressl/pkgconfig"
```

<!--
### Mac OS

1. Install all the development requirements:

```sh
brew install cmake make gcc pkg-config openssl@3 boost gtkmm4
```

2. Export variables:

```sh
export PATH="/usr/local/opt/make/libexec/gnubin:$PATH"
export PATH="/usr/local/opt/libressl/bin:$PATH"
export LDFLAGS="-L/usr/local/opt/libressl/lib"
export CPPFLAGS="-I/usr/local/opt/libressl/include"
export PKG_CONFIG_PATH="/usr/local/opt/libressl/lib/pkgconfig"
```
-->

## Build Instructions

1. Clone this repo:

```sh
git clone https://github.com/grobx/udacity-CppND-final-project
```

2. Make a build directory in the top level directory:

```sh
mkdir build && cd build
```

3. Compile:

```sh
cmake .. && make
```

## Running

1. Set the dictionary API key (go to https://www.dictionaryapi.com/ to obtain one):

```sh
export DICTIONARY_API_KEY="aaa-bbb-ccc"
```

2. Run it:

```sh
./Dictionary
```

## TODO

- [ ] meet all required requirements
- [ ] meet all addressed requirements
- [x] add debug logs
- [ ] improve exception handling
- [ ] check that you can build and run this app on Arch Linux and Windows

## Rubric Requirments

### Required

#### README

- [x] The README.md is included with the project and has instructions for building/running the project.
- [x] If any additional libraries are needed to run the project, these are indicated with cross-platform installation instructions.
- [x] The README describes the project you have built.
- [ ] The README also indicates the file and class structure, along with the expected behavior or output of the program.
- [ ] The README indicates which rubric points are addressed. The README also indicates where in the code (i.e. files and line numbers) that the rubric points are addressed.

#### Compiling and Testing

- [ ] The project code must compile and run without errors. 

### Choosed

#### Loops, Functions, I/O

- [ ] A variety of control structures are used in the project.

#### Object Oriented Programming

- [ ] The project code is organized into classes with class attributes to hold the data, and class methods to perform tasks.
- [ ] All class data members are explicitly specified as public, protected, or private.
- [ ] All class members that are set to argument values are initialized through member initialization lists.
- [ ] All class member functions document their effects, either through function names, comments, or formal documentation. Member functions do not change program state in undocumented ways.
- [ ] Appropriate data and functions are grouped into classes. Member data that is subject to an invariant is hidden from the user. State is accessed via member functions.
- [ ] One function is declared with a template that allows it to accept a generic parameter.

#### Memory Management

- [ ] At least two variables are defined as references, or two functions use pass-by-reference in the project code.
- [ ] The project uses at least one smart pointer: unique_ptr, shared_ptr, or weak_ptr. The project does not use raw pointers.

#### Concurrency

- [ ] The project uses multiple threads in the execution.
