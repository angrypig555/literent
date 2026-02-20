#!/bin/bash
echo "Build script only compatible with RHEL based distros"
sudo dnf install -y rb_libtorrent-devel glfw-devel mesa-libGL-devel gcc-c++ cmake

cmake --preset gc
cmake --build out/build/gc/
out/build/gc/./literent