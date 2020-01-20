#!/usr/bin/env bash

sudo apt-get install bzip2
yes | sudo apt install python-pip
pip install pyserial
pip install --upgrade pip

echo " > wget -O arduino-cli-linux64.tar.gz https://github.com/arduino/arduino-cli/releases/download/0.7.2/arduino-cli_0.7.2_Linux_64bit.tar.gz"
wget -O arduino-cli-linux64.tar.gz https://github.com/arduino/arduino-cli/releases/download/0.7.2/arduino-cli_0.7.2_Linux_64bit.tar.gz

echo " > tar -zxvf arduino-cli-linux64.tar.gz"
tar -zxvf arduino-cli-linux64.tar.gz

echo " > sudo mv arduino-cli /usr/local/share/arduino-cli"
sudo mv arduino-cli /usr/local/share/arduino-cli

echo " > sudo ln -s /usr/local/share/arduino-cli /usr/local/bin/arduino-cli"
sudo ln -s /usr/local/share/arduino-cli /usr/local/bin/arduino-cli

printf "board_manager:
  additional_urls:
    - https://dl.espressif.com/dl/package_esp32_index.json" >>.cli-config.yml
sudo mv .cli-config.yml /usr/local/share/

arduino-cli core update-index
arduino-cli core install arduino:avr
