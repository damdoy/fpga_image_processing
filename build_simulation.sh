#!/bin/bash
set -e
#compile simple_cpu

if [ -d "obj_dir" ]; then
  rm -r obj_dir
fi

# to create the obj dir:
verilator -Wall --cc hdl/image_processing.v --exe simulation/image_processing_simulation.cpp software/main.cpp 

# to compile:
make CXXFLAGS="-g" -j -C obj_dir -f Vimage_processing.mk Vimage_processing

cp obj_dir/Vimage_processing simu
