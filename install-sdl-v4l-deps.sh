#!/bin/bash

# Installing the v4l-utils development packages.
echo "---------------------------------------------> Installing v4l-utils!"
sudo apt-get install v4l-utils

echo "---------------------------------------------> Installing SDL2 and SDL_Image libraries"
echo "[+] -> Installing SDL2"
sudo apt install libsdl2-dev libsdl2-2.0-0

echo "[+] -> Installing SDL2_Image"
sudo apt install libjpeg9-dev libwebp-dev libtiff5-dev libsdl2-image-dev libsdl2-image-2.0-0

echo "---------------------------------------------> All Done!"
