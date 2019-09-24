#include <cstdio>
#include <queue>

// #include "image_fruits_128.h"
#include "image_fruits_64.h"
// #include "image_fruits_8.h"
// #include "image_sequential.h"

#include "../obj_dir/Vimage_processing.h"

#define SIZE_IMAGE 8

uint16_t memory[512*128]; //128k memory

enum Commands {COMMAND_PARAM, COMMAND_SEND_IMG, COMMAND_READ_IMG, COMMAND_GET_STATUS, COMMAND_APPLY_ADD, COMMAND_APPLY_THRESHOLD,
               COMMAND_SWITCH_BUFFERS, COMMAND_BINARY_ADD, COMMAND_APPLY_INVERT, COMMAND_APPLY_POW, COMMAND_APPLY_SQRT,
               COMMAND_CONVOLUTION, COMMAND_BINARY_SUB, COMMAND_BINARY_MULT, COMMAND_NONE=255};

struct Operation{
   bool is_command;
   Commands command;
   uint8_t data;

   Operation(bool is_command, Commands com, uint8_t data){
      this->is_command = is_command;
      this->command = com;
      this->data = data;
   }
};

typedef std::queue<Operation> FIFO_OP;

std::queue<uint16_t> fifo_out;

//simulates the I/O of the fpga controller
//ram and "communication" (which should be SPI)
void main_loop_clk(Vimage_processing *tb, FIFO_OP *fifo){
   tb->clk = 0;
   tb->reset = 0;
   tb->eval();
   tb->clk = 1;
   tb->reset = 0;
   tb->comm_data_in_valid = 0;
   if(!fifo->empty()){
      Operation op = fifo->front();
      fifo->pop();
      if(op.is_command){
         tb->comm_data_in_valid = 1;
         tb->comm_cmd = op.command;
      }
      else{
         tb->comm_data_in_valid = 1;
         tb->comm_data_in = op.data;
      }
   }

   if(tb->comm_data_out_valid == 1){
      fifo_out.push(tb->comm_data_out);
   }

   tb->data_read_valid = 0;
   if(tb->rd_en == 1){
      printf("read req addr: 0x%x\n", tb->addr);
      tb->data_read = memory[tb->addr/2];
      tb->data_read_valid = 1;
   }
   if(tb->wr_en == 1){
      printf("wants to write: addr:0x%x data:0x%x\n", tb->addr, tb->data_write);
      memory[tb->addr/2] = tb->data_write;
   }

   tb->eval();
}

bool send_params(Vimage_processing *tb, uint16_t img_width16, uint16_t img_height16, FIFO_OP *fifo){
   uint8_t img_width8[2] = {img_width16&0xFF, (img_width16>>8)&0xFF};
   uint8_t img_height8[2] = {img_height16&0xFF, (img_height16>>8)&0xFF};

   printf("sending img_width8[0]: %u, [1]: %u\n", img_width8[0], img_width8[1]);
   printf("sending img_height8[0]: %u, [1]: %u\n", img_height8[0], img_height8[1]);

   fifo->push(Operation(true, COMMAND_PARAM, 0));
   fifo->push(Operation(false, COMMAND_NONE, img_height8[0]));
   fifo->push(Operation(false, COMMAND_NONE, img_height8[1]));
   fifo->push(Operation(false, COMMAND_NONE, img_width8[0]));
   fifo->push(Operation(false, COMMAND_NONE, img_width8[1]));

   for (size_t i = 0; i < 5; i++) {
      main_loop_clk(tb, fifo);
   }
}

void read_status(Vimage_processing *tb, FIFO_OP *fifo){
   fifo->push(Operation(true, COMMAND_GET_STATUS, 0));

   for (size_t i = 0; i < 6; i++) {
      main_loop_clk(tb, fifo);
   }

   while(!fifo_out.empty()){
      printf("read status: %u\n", fifo_out.front());
      fifo_out.pop();
   }
}

//sends a sequential image (each pixel is the increment of the previous one)
void send_image(Vimage_processing *tb, FIFO_OP *fifo){
   fifo->push(Operation(true, COMMAND_SEND_IMG, 0));
   for (size_t i = 0; i < SIZE_IMAGE*SIZE_IMAGE; i++) {
      fifo->push(Operation(false, COMMAND_NONE, i+1));
   }

   for (size_t i = 0; i < SIZE_IMAGE*SIZE_IMAGE+2; i++) {
      main_loop_clk(tb, fifo);
   }
}

void send_image(Vimage_processing *tb, FIFO_OP *fifo, uint8_t *image){
   fifo->push(Operation(true, COMMAND_SEND_IMG, 0));
   for (size_t i = 0; i < image_width*image_height; i++) {
      fifo->push(Operation(false, COMMAND_NONE, image[i]));
   }

   for (size_t i = 0; i < image_width*image_height+500; i++) {
      main_loop_clk(tb, fifo);
   }
}

void send_image_add(Vimage_processing *tb, int16_t add_value16, bool clamp, FIFO_OP *fifo){

   uint8_t add_value8[2] = {add_value16&0xFF, (add_value16>>8)&0xFF};

   fifo->push(Operation(true, COMMAND_APPLY_ADD, 0));
   fifo->push(Operation(false, COMMAND_NONE, add_value8[0]));
   fifo->push(Operation(false, COMMAND_NONE, add_value8[1]));
   fifo->push(Operation(false, COMMAND_NONE, clamp));

   for (size_t i = 0; i < 100; i++) { //don't wait too long, will be checked with status
      main_loop_clk(tb, fifo);
   }
}

void send_image_threshold(Vimage_processing *tb, uint8_t threshold_value, uint8_t replacement_value, bool upper_selection, FIFO_OP *fifo){

   fifo->push(Operation(true, COMMAND_APPLY_THRESHOLD, 0));
   fifo->push(Operation(false, COMMAND_NONE, threshold_value));
   fifo->push(Operation(false, COMMAND_NONE, replacement_value));
   fifo->push(Operation(false, COMMAND_NONE, upper_selection));

   for (size_t i = 0; i < 500; i++) {
      main_loop_clk(tb, fifo);
   }
}

void send_image_invert(Vimage_processing *tb, FIFO_OP *fifo){

   fifo->push(Operation(true, COMMAND_APPLY_INVERT, 0));

   for (size_t i = 0; i < 10; i++) {
      main_loop_clk(tb, fifo);
   }
}

void send_pow(Vimage_processing *tb, FIFO_OP *fifo){
   fifo->push(Operation(true, COMMAND_APPLY_POW, 0));

   for (size_t i = 0; i < 10; i++) {
      main_loop_clk(tb, fifo);
   }
}

void send_sqrt(Vimage_processing *tb, FIFO_OP *fifo){
   fifo->push(Operation(true, COMMAND_APPLY_SQRT, 0));

   for (size_t i = 0; i < 10; i++) {
      main_loop_clk(tb, fifo);
   }
}

void read_image(Vimage_processing *tb, FIFO_OP *fifo){

   fifo->push(Operation(true, COMMAND_READ_IMG, 0));

   for (size_t i = 0; i < 500; i++) {
      main_loop_clk(tb, fifo);
   }

   uint count = 0;
   while(!fifo_out.empty()){
      printf("read image: %u, %u\n", count, fifo_out.front());
      count++;
      fifo_out.pop();
   }
}

void read_image(Vimage_processing *tb, FIFO_OP *fifo, uint8_t* image_out){

   fifo->push(Operation(true, COMMAND_READ_IMG, 0));

   for (size_t i = 0; i < image_width*image_height*2; i++) {
      main_loop_clk(tb, fifo);
   }

   uint count = 0;
   while(!fifo_out.empty()){
      printf("read image: %u, %u\n", count, fifo_out.front());
      image_out[count] = fifo_out.front();
      count++;
      fifo_out.pop();
   }
}

void switch_buffers(Vimage_processing *tb, FIFO_OP *fifo){
   fifo->push(Operation(true, COMMAND_SWITCH_BUFFERS, 0));

   for (size_t i = 0; i < 10; i++) {
      main_loop_clk(tb, fifo);
   }
}

//will add the input buffer with the storage buffer
void send_binary_add(Vimage_processing *tb, bool clamp, FIFO_OP *fifo){
   fifo->push(Operation(true, COMMAND_BINARY_ADD, 0));
   fifo->push(Operation(false, COMMAND_NONE, clamp));

   for (size_t i = 0; i < 500; i++) {
      main_loop_clk(tb, fifo);
   }
}

//loop until not busy
void wait_end_busy(Vimage_processing *tb, FIFO_OP *fifo){

   uint8_t status_out[4];

   do{
      fifo->push(Operation(true, COMMAND_GET_STATUS, 0));

      for (size_t i = 0; i < 10; i++) {
         main_loop_clk(tb, fifo);
      }

      for (size_t i = 0; i < 4; i++) {
         status_out[i] = fifo_out.front();
         printf("status %d = %d\n", i, fifo_out.front());
         fifo_out.pop();
      }
      printf("wait_end_busy\n");
   } while( (status_out[0]&0x01 == 1) );
}

void send_convolution(Vimage_processing *tb, FIFO_OP *fifo, uint8_t *conv_kernel, bool clamp, bool input_source, bool add_to_output){
   fifo->push(Operation(true, COMMAND_CONVOLUTION, 0));
   //parameters
   fifo->push(Operation(false, COMMAND_NONE, (add_to_output<<2)+(input_source<<1)+clamp));

   for (size_t i = 0; i < 9; i++) {
      fifo->push(Operation(false, COMMAND_NONE, conv_kernel[i]));
   }

   for (size_t i = 0; i < 100; i++) {
      main_loop_clk(tb, fifo);
   }
}

void send_clear(Vimage_processing *tb, FIFO_OP *fifo, uint8_t value){

   //everything >= 0 will be replaced with "value"
   send_image_threshold(tb, 0, value, false, fifo);
}

void test_add_threshold(Vimage_processing *tb, FIFO_OP *fifo_commands, uint8_t *image_input, uint8_t *image_output){
   send_params(tb, image_width, image_height, fifo_commands);

   read_status(tb, fifo_commands);

   send_image(tb, fifo_commands, image_input);

   read_status(tb, fifo_commands);
   printf("===========ADD===========\n");
   switch_buffers(tb, fifo_commands);
   send_image_add(tb, 32, true, fifo_commands);
   wait_end_busy(tb, fifo_commands);

   printf("===========THRESHOLD===========\n");
   send_image_threshold(tb, 168, 0, 0, fifo_commands);
   wait_end_busy(tb, fifo_commands);

   switch_buffers(tb, fifo_commands);
   read_image(tb, fifo_commands, image_output);
}

//will load the image in the input buffer and set the storage buffer to pixels of value 32
// and then will add the two buffers
void test_binary_add(Vimage_processing *tb, FIFO_OP *fifo_commands, uint8_t *image_input, uint8_t *image_output){
   send_params(tb, image_width, image_height, fifo_commands);
   send_image(tb, fifo_commands, image_input); //in input buffer

   send_clear(tb, fifo_commands, 32); //storage buffer will have an image full of pixels 32
   wait_end_busy(tb, fifo_commands);

   switch_buffers(tb, fifo_commands);

   send_binary_add(tb, true, fifo_commands);
   wait_end_busy(tb, fifo_commands);

   switch_buffers(tb, fifo_commands);
   read_image(tb, fifo_commands, image_output);

}

void test_gaussian_blur(Vimage_processing *tb, FIFO_OP *fifo_commands, uint8_t *image_input, uint8_t *image_output){
   send_params(tb, image_width, image_height, fifo_commands);
   send_image(tb, fifo_commands, image_input);

   //gaussian blur kernel
   uint8_t conv_kernel[9] = {(1)<<0, (1)<<1, (1)<<0, (1)<<1, ((1)<<2), (1)<<1,  (1)<<0, (1)<<1, (1)<<0};

   // uint8_t conv_kernel[9] = {(1)<<4, (1)<<4, (1)<<4, (1)<<4, ((-7)<<4), (1)<<4,  (1)<<4, (1)<<4, (1)<<4};

   send_convolution(tb, fifo_commands, conv_kernel, true, true, false);

   printf("wait end busy\n");

   wait_end_busy(tb, fifo_commands);

   printf("switch buffers\n");

   switch_buffers(tb, fifo_commands);

   read_image(tb, fifo_commands, image_output);
}

void test_sobel_simple(Vimage_processing *tb, FIFO_OP *fifo_commands, uint8_t *image_input, uint8_t *image_output){
   send_params(tb, image_width, image_height, fifo_commands);
   send_image(tb, fifo_commands, image_input);

   //top sobel
   {
      uint8_t conv_kernel[9] = {(1)<<3, (1)<<4, (1)<<3, (0)<<4, ((0)<<4), (0)<<4,  (-1)<<3, (-1)<<4, (-1)<<3};
      send_convolution(tb, fifo_commands, conv_kernel, true, true, true);
   }
   wait_end_busy(tb, fifo_commands);

   //bottom sobel
   {
      uint8_t conv_kernel[9] = {(-1)<<3, (-1)<<4, (-1)<<3, (0)<<4, ((0)<<4), (0)<<4,  (1)<<3, (1)<<4, (1)<<3};
      send_convolution(tb, fifo_commands, conv_kernel, true, true, true);
   }
   wait_end_busy(tb, fifo_commands);

   //left sobel
   {
      uint8_t conv_kernel[9] = {(1)<<3, (0)<<4, (-1)<<3, (1)<<4, ((0)<<4), (-1)<<4,  (1)<<3, (0)<<4, (-1)<<3};
      send_convolution(tb, fifo_commands, conv_kernel, true, true, true);
   }
   wait_end_busy(tb, fifo_commands);

   //right sobel
   {
      uint8_t conv_kernel[9] = {(-1)<<3, (0)<<4, (1)<<3, (-1)<<4, ((0)<<4), (1)<<4,  (-1)<<3, (0)<<4, (1)<<3};
      send_convolution(tb, fifo_commands, conv_kernel, true, true, true);
   }
   wait_end_busy(tb, fifo_commands);

   switch_buffers(tb, fifo_commands);

   read_image(tb, fifo_commands, image_output);
}

int main(){

   FIFO_OP fifo_commands;
   FILE *output_file;
   output_file = fopen("output.dat", "w");

   // Create an instance of our module under test
   Vimage_processing *tb = new Vimage_processing;

   uint8_t *image_input = new uint8_t[image_width*image_height];
   uint8_t *image_output = new uint8_t[image_width*image_height];

   char *ptr_image = header_data;
   for (size_t i = 0; i < image_height*image_width; i++) {
      uint8_t pixel[3];
      HEADER_PIXEL(ptr_image, pixel);
      image_input[i] = pixel[0];
   }

   //test selection
   // test_add_threshold(tb, &fifo_commands, image_input, image_output);
   // test_binary_add(tb, &fifo_commands, image_input, image_output);
   test_gaussian_blur(tb, &fifo_commands, image_input, image_output);
   // test_sobel_simple(tb, &fifo_commands, image_input, image_output);

   for (size_t i = 0; i < image_height*image_width; i++) {
      fprintf(output_file, "%d ", image_output[i]);
      if( ((i+1) % (image_width)) == 0){
         fprintf(output_file, "\n");
      }
   }

   fclose(output_file);

   return 0;
}
