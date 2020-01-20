#!/usr/bin/env bash

arduino-cli compile --output sketch -b arduino:avr:mini $PWD/src/thermo.cpp
