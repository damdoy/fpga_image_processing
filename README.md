# fpga_image_processing

## WIP

Image processing operations done on a fpga, simulated with verilator and running
on a ice40 ultraplus.

It is targeted to low end fpga devices. It uses 1Mbit of ram to store the images into
two buffers, the input and the storage buffer.
Images are loaded and read in the input buffer, the two buffers can be swapped.

Some operations will be done on one of the buffers (addition or convolution)
while other will combine the two buffers (add/sub).

The software communicates with the simulated fpga controller via a communication channel and will respond to memory access.

The goal in the future is to have the communication channel done with a SPI module and the memory access with the memory available to the ice40 fpga.

The images are stored in a .h (done with gimp).

### Operations
- per pixel ops
   - add/sub
   - invert
   - threshold
   - pow2
   - sqrt
- 3x3 matrix convolution
- combine input and buffer
   - add
   - sub
   - mult
-- switch input and buffer

### Commands
- format: command+data_to_receive-data_to_send
- set_params/init, command+width[15:0] height[15:0]-
- send_image, command+image[width*height:0]-
- read_image, command+-image[width*height:0]
- get_status command+-status[31:0]
- apply_add command+val[15:0]
- threshold command+val[7:0]+replacement[7:0]+upper[0] (upper == 1 will replace everything >val with replacement)

# Build & run

`./build_simulation.sh` Builds the simulation and C program

`./simu` Runs simulator

`./run_gnuplot.sh` to display the output (`output.dat`) in gnuplot

## Needed tools

Verilator 3.874

gnuplot 5.0
