#include <cstdio>
#include <queue>

#include "image_fruits.h"
// #include "image_fruits_8.h"
// #include "image_sequential.h"

#include "../obj_dir/Vcontroller.h"

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


void main_loop_clk(Vcontroller *tb, FIFO_OP *fifo){
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

bool send_params(Vcontroller *tb, uint16_t img_width16, uint16_t img_height16, FIFO_OP *fifo){
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

   // tb->comm_data_in_valid = 0;
}

void read_status(Vcontroller *tb, FIFO_OP *fifo){
   fifo->push(Operation(true, COMMAND_GET_STATUS, 0));

   for (size_t i = 0; i < 6; i++) {
      main_loop_clk(tb, fifo);
   }

   while(!fifo_out.empty()){
      printf("read status: %u\n", fifo_out.front());
      fifo_out.pop();
   }
}

void send_image(Vcontroller *tb, FIFO_OP *fifo){
   fifo->push(Operation(true, COMMAND_SEND_IMG, 0));
   for (size_t i = 0; i < SIZE_IMAGE*SIZE_IMAGE; i++) {
      fifo->push(Operation(false, COMMAND_NONE, i+1));
   }

   for (size_t i = 0; i < SIZE_IMAGE*SIZE_IMAGE+2; i++) {
      main_loop_clk(tb, fifo);
   }
}

void send_image(Vcontroller *tb, FIFO_OP *fifo, uint8_t *image){
   fifo->push(Operation(true, COMMAND_SEND_IMG, 0));
   for (size_t i = 0; i < image_width*image_height; i++) {
      fifo->push(Operation(false, COMMAND_NONE, image[i]));
   }

   for (size_t i = 0; i < image_width*image_height+500; i++) {
      main_loop_clk(tb, fifo);
   }
}

void send_image_add(Vcontroller *tb, int16_t add_value16, bool clamp, FIFO_OP *fifo){

   uint8_t add_value8[2] = {add_value16&0xFF, (add_value16>>8)&0xFF};

   fifo->push(Operation(true, COMMAND_APPLY_ADD, 0));
   fifo->push(Operation(false, COMMAND_NONE, add_value8[0]));
   fifo->push(Operation(false, COMMAND_NONE, add_value8[1]));
   fifo->push(Operation(false, COMMAND_NONE, clamp));

   for (size_t i = 0; i < 100; i++) { //don't wait too long, will be checked with status
      main_loop_clk(tb, fifo);
   }
}

void send_image_threshold(Vcontroller *tb, uint8_t threshold_value, uint8_t replacement_value, bool upper_selection, FIFO_OP *fifo){

   fifo->push(Operation(true, COMMAND_APPLY_THRESHOLD, 0));
   fifo->push(Operation(false, COMMAND_NONE, threshold_value));
   fifo->push(Operation(false, COMMAND_NONE, replacement_value));
   fifo->push(Operation(false, COMMAND_NONE, upper_selection));

   for (size_t i = 0; i < 500; i++) {
      main_loop_clk(tb, fifo);
   }
}

void send_image_invert(Vcontroller *tb, FIFO_OP *fifo){

   fifo->push(Operation(true, COMMAND_APPLY_INVERT, 0));

   for (size_t i = 0; i < 10; i++) {
      main_loop_clk(tb, fifo);
   }
}

void send_pow(Vcontroller *tb, FIFO_OP *fifo){
   fifo->push(Operation(true, COMMAND_APPLY_POW, 0));

   for (size_t i = 0; i < 10; i++) {
      main_loop_clk(tb, fifo);
   }
}

void send_sqrt(Vcontroller *tb, FIFO_OP *fifo){
   fifo->push(Operation(true, COMMAND_APPLY_SQRT, 0));

   for (size_t i = 0; i < 10; i++) {
      main_loop_clk(tb, fifo);
   }
}

void read_image(Vcontroller *tb, FIFO_OP *fifo){

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

void read_image(Vcontroller *tb, FIFO_OP *fifo, uint8_t* image_out){

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

void switch_buffers(Vcontroller *tb, FIFO_OP *fifo){
   fifo->push(Operation(true, COMMAND_SWITCH_BUFFERS, 0));

   for (size_t i = 0; i < 10; i++) {
      main_loop_clk(tb, fifo);
   }
}

void send_binary_add(Vcontroller *tb, bool clamp, FIFO_OP *fifo){
   fifo->push(Operation(true, COMMAND_BINARY_ADD, 0));
   fifo->push(Operation(false, COMMAND_NONE, clamp));

   for (size_t i = 0; i < 500; i++) {
      main_loop_clk(tb, fifo);
   }
}

void wait_end_busy(Vcontroller *tb, FIFO_OP *fifo){

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

void send_convolution(Vcontroller *tb, FIFO_OP *fifo, uint8_t *conv_kernel, bool clamp, bool input_source){
   fifo->push(Operation(true, COMMAND_CONVOLUTION, 0));
   //parameters
   fifo->push(Operation(false, COMMAND_NONE, (input_source<<1)+clamp));

   for (size_t i = 0; i < 9; i++) {
      fifo->push(Operation(false, COMMAND_NONE, conv_kernel[i]));
   }

   for (size_t i = 0; i < 100; i++) {
      main_loop_clk(tb, fifo);
   }
}

void test1(Vcontroller *tb, FIFO_OP *fifo_commands){
   send_params(tb, SIZE_IMAGE, SIZE_IMAGE, fifo_commands);

   read_status(tb, fifo_commands);

   send_image(tb, fifo_commands);

   read_status(tb, fifo_commands);
   printf("===========ADD===========\n");
   switch_buffers(tb, fifo_commands);
   send_image_add(tb, 1, true, fifo_commands);

   switch_buffers(tb, fifo_commands);
   read_image(tb, fifo_commands);
   switch_buffers(tb, fifo_commands);
   printf("===========THRESHOLD===========\n");
   send_image_threshold(tb, 32, 0, 0, fifo_commands);

   switch_buffers(tb, fifo_commands);
   read_image(tb, fifo_commands);
}

void test2(Vcontroller *tb, FIFO_OP *fifo_commands){
   //load image in input buffer, apply add, switch buffer load other image, threshold, switch, read, switch read
   send_params(tb, SIZE_IMAGE, SIZE_IMAGE, fifo_commands);
   send_image(tb, fifo_commands);
   switch_buffers(tb, fifo_commands);
   send_image_add(tb, 16, true, fifo_commands);

   send_image(tb, fifo_commands);
   switch_buffers(tb, fifo_commands);
   send_image_threshold(tb, 32, 128, 1, fifo_commands);

   read_image(tb, fifo_commands);
   switch_buffers(tb, fifo_commands);
   read_image(tb, fifo_commands);

}

void test3(Vcontroller *tb, FIFO_OP *fifo_commands){
   send_params(tb, SIZE_IMAGE, SIZE_IMAGE, fifo_commands);
   send_image(tb, fifo_commands);
   switch_buffers(tb, fifo_commands);
   send_image_add(tb, 16, true, fifo_commands);


   send_image(tb, fifo_commands);
   switch_buffers(tb, fifo_commands);
   send_image_threshold(tb, 32, 128, 1, fifo_commands);

   send_binary_add(tb, true, fifo_commands);

   switch_buffers(tb, fifo_commands);
   read_image(tb, fifo_commands);
   // switch_buffers(tb, fifo_commands);
   // read_image(tb, fifo_commands);
}

void test4(Vcontroller *tb, FIFO_OP *fifo_commands, uint8_t *image_input, uint8_t *image_output){
   send_params(tb, image_width, image_height, fifo_commands);
   send_image(tb, fifo_commands, image_input);

   switch_buffers(tb, fifo_commands);

   // send_image_threshold(tb, 128, 255, 1, fifo_commands);
   send_image_add(tb, -128, true, fifo_commands);
   // send_image_invert(tb, fifo_commands);

   wait_end_busy(tb, fifo_commands);

   switch_buffers(tb, fifo_commands);

   read_image(tb, fifo_commands, image_output);
}

void test_convolution(Vcontroller *tb, FIFO_OP *fifo_commands, uint8_t *image_input, uint8_t *image_output){
   send_params(tb, image_width, image_height, fifo_commands);
   send_image(tb, fifo_commands, image_input);

   // switch_buffers(tb, fifo_commands);

   // send_image_threshold(tb, 128, 255, 1, fifo_commands);
   // send_image_add(tb, -128, true, fifo_commands);
   // send_image_invert(tb, fifo_commands);

   //gaussian blur
   // uint8_t conv_kernel[9] = {(1)<<0, (1)<<1, (1)<<0, (1)<<1, ((1)<<2), (1)<<1,  (1)<<0, (1)<<1, (1)<<0};
   uint8_t conv_kernel[9] = {(1)<<0, (1)<<1, (1)<<0, (1)<<1, ((1)<<2), (1)<<1,  (1)<<0, (1)<<1, (1)<<0};

   send_convolution(tb, fifo_commands, conv_kernel, true, true);
   // send_image_add(tb, 0, true, fifo_commands);

   printf("wait end busy\n");

   wait_end_busy(tb, fifo_commands);

   printf("switch buffers\n");

   switch_buffers(tb, fifo_commands);

   read_image(tb, fifo_commands, image_output);
}

int main(){
   // Initialize Verilators variables
   // Verilated::commandArgs(argc, argv);

   FIFO_OP fifo_commands;
   FILE *output_file;
   output_file = fopen("output.dat", "w");

   // Create an instance of our module under test
   Vcontroller *tb = new Vcontroller;

   uint8_t *image_input = new uint8_t[image_width*image_height];
   uint8_t *image_output = new uint8_t[image_width*image_height];

   char *ptr_image = header_data;
   for (size_t i = 0; i < image_height*image_width; i++) {
      uint8_t pixel[3];
      HEADER_PIXEL(ptr_image, pixel);
      image_input[i] = pixel[0];
   }

   // test1(tb, &fifo_commands);
   // test2(tb, &fifo_commands);
   // test3(tb, &fifo_commands);
   // test4(tb, &fifo_commands, image_input, image_output);
   test_convolution(tb, &fifo_commands, image_input, image_output);

   for (size_t i = 0; i < image_height*image_width; i++) {
      fprintf(output_file, "%d ", image_output[i]);
      if( ((i+1) % (image_width)) == 0){
         fprintf(output_file, "\n");
      }
   }

   fclose(output_file);

   return 0;
}
