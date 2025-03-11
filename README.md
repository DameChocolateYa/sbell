# Sbell

Sbell is a minimalist Interpreter to handle basic linux terminal operations. It is completly written in C++. By the moment it's only available in arch linux, but it's expected to to arrive in Debian or derivated systems of Debian.

## Installation

* Arch Linux: `yay -S sbell` or `paru -S sbell`

You may need `sudo`

## Modify and compile
if you want to modify sbell or you want to install it compiling you need to do the next:

* You need to install make, cmake and nlohmann-json
* Download source code, you can do it executing `git clone https://github.com/DameChocolateYa/sbel.git` (you can also download it from releases)
* Unzip the source code
* (OPTIONAL) modify the source code, the main files are in src/, the header files are in src/include/ and the lang files are in lang/
* Compile it: first, being in parent dir you have to create a directory for the building (we are going to call it build/), then move to that dir, and execute `cmake ..`, this will generate some files, then execute `make`, when it is done, there will a executable called sbell, you can move it to /usr/bin/
* NOTE: if you download the interpreter with this method, you will need to move the lang dir to /etc/sbell/lang (creating the directories), if you don't do it, the interpreter will have some lang problems (the traductions which sbell cant find will replace it with english, but if sbell also can't find the english file, it will replace the message with ??ID)

## Q: How can I use it?

* If you want to use it as main interpreter you need to: Edit /etc/shells and add this line /usr/bin/sbell | Put this on terminal `chsh -s $(which sbell)`

* Config file is located into ~/.sbellrc and it is created when you init for first time sbell

## Syntax and new things:

* Set variables: to create a variable you have to execute: `export VARIABLE VALUE`
* Get variables: to get a variable you have to execute: `$:VARIABLE$`
* Get command output: to get the output of a command you've to execute: `$(COMMAND)`
* Create alias: to create a alias you need to execute: `alias ALIAS_NAME ALIAS_VALUE`

if you want to preservate set variables and alias changes you hace to add at the end `--all-sessions`

## Purpose

This project is made for educational purposes and is not recommended use it as main interpreter

## Upcoming features

The upcoming and basic features which are expected to be developed are:
 
* Integrated words like: if, for...
* Files like .sh
* Colorful Syntax
* Autocompletation
* ...

## Third-party Libraries

This project uses the following third-party libraries:

- **[nlohmann-json](https://github.com/nlohmann/json)** - Licensed under the MIT License.
