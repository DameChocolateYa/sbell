# Sbell

Sbell is a minimalist Interpreter to handle basic linux terminal operations. It is completly written in C++. By the moment it's only available in arch linux, but it's expecto to arrive in Debian or derivated systems of Debian.

## Installation

* Arch Linux: `yay -S sbell` or `paru -S sbell`

You may need `sudo`

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

* Redirections (like: > or 2>&1)
* Pipes (|)
* Boolean Operators (like: && or ||)
* Integrated words like: if, for...
* Files like .sh
* ...

## Third-party Libraries

This project uses the following third-party libraries:

- **[nlohmann-json](https://github.com/nlohmann/json)** - Licensed under the MIT License.
