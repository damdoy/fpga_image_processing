#!/bin/bash
set -e
#compile simple_cpu

if [ -d "obj_dir" ]; then
  rm -r obj_dir
fi

# to create the obj dir:
verilator -Wall --cc hdl/controller.v --exe simulation/main.cpp

# to compile:
make CXXFLAGS="-g" -j -C obj_dir -f Vcontroller.mk Vcontroller

cp obj_dir/Vcontroller simu
