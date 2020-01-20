#!/usr/bin/env bash

arduino-cli compile --output temp.bin -b esp32:esp32:esp32 $PWD/Test/Test.ino --debug
