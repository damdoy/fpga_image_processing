#include <stdint.h>
#include <queue>
#include <cmath>

#include "image_processing_simulation.hpp"

Image_processing_simulation::Image_processing_simulation(){
   this->simulator = new Vimage_processing;

   this->memory = new uint16_t[512*128];
}

Image_processing_simulation::~Image_processing_simulation(){
   delete this->simulator;
   delete[] this->memory;
}

void Image_processing_simulation::send_params(uint16_t img_width, uint16_t img_height){

   this->image_width = img_width;
   this->image_height = img_height;

   uint8_t img_width8[2] = {img_width&0xFF, (img_width>>8)&0xFF};
   uint8_t img_height8[2] = {img_height&0xFF, (img_height>>8)&0xFF};

   printf("sending img_width8[0]: %u, [1]: %u\n", img_width8[0], img_width8[1]);
   printf("sending img_height8[0]: %u, [1]: %u\n", img_height8[0], img_height8[1]);

   fifo_in.push(Operation(true, COMMAND_PARAM, 0));
   fifo_in.push(Operation(false, COMMAND_NONE, img_width8[0]));
   fifo_in.push(Operation(false, COMMAND_NONE, img_width8[1]));
   fifo_in.push(Operation(false, COMMAND_NONE, img_height8[0]));
   fifo_in.push(Operation(false, COMMAND_NONE, img_height8[1]));

   for (size_t i = 0; i < 5; i++) {
      main_loop_clk();
   }
}

void Image_processing_simulation::read_status(uint8_t *output){
   fifo_in.push(Operation(true, COMMAND_GET_STATUS, 0));

   for (size_t i = 0; i < 100; i++) {
      main_loop_clk();
   }

   // while(!fifo_out.empty()){
   //    printf("read status: %u\n", fifo_out.front());
   //    fifo_out.pop();
   // }

   for (size_t i = 0; i < 4; i++) {
      if(!fifo_out.empty()){
         printf("read status: %u\n", fifo_out.front());
         output[i] = fifo_out.front();
         fifo_out.pop();
      }
   }
}

void Image_processing_simulation::send_image(uint8_t *image){
   fifo_in.push(Operation(true, COMMAND_SEND_IMG, 0));
   for (size_t i = 0; i < image_width*image_height; i++) {
      fifo_in.push(Operation(false, COMMAND_NONE, image[i]));
   }

   for (size_t i = 0; i < image_width*image_height+500; i++) {
      main_loop_clk();
   }
}

void Image_processing_simulation::send_add(int16_t value, bool clamp){
   uint8_t add_value8[2] = {value&0xFF, (value>>8)&0xFF};

   fifo_in.push(Operation(true, COMMAND_APPLY_ADD, 0));
   fifo_in.push(Operation(false, COMMAND_NONE, add_value8[0]));
   fifo_in.push(Operation(false, COMMAND_NONE, add_value8[1]));
   fifo_in.push(Operation(false, COMMAND_NONE, clamp));

   for (size_t i = 0; i < 100; i++) { //don't wait too long, will be checked with status
      main_loop_clk();
   }
}

void Image_processing_simulation::send_threshold(uint8_t threshold_value, uint8_t replacement_value, bool upper_selection){
   fifo_in.push(Operation(true, COMMAND_APPLY_THRESHOLD, 0));
   fifo_in.push(Operation(false, COMMAND_NONE, threshold_value));
   fifo_in.push(Operation(false, COMMAND_NONE, replacement_value));
   fifo_in.push(Operation(false, COMMAND_NONE, upper_selection));

   for (size_t i = 0; i < 100; i++) {
      main_loop_clk();
   }
}

void Image_processing_simulation::send_image_invert(){
   fifo_in.push(Operation(true, COMMAND_APPLY_INVERT, 0));

   for (size_t i = 0; i < 10; i++) {
      main_loop_clk();
   }
}

void Image_processing_simulation::send_pow(){
   fifo_in.push(Operation(true, COMMAND_APPLY_POW, 0));

   for (size_t i = 0; i < 10; i++) {
      main_loop_clk();
   }
}

void Image_processing_simulation::send_sqrt(){
   fifo_in.push(Operation(true, COMMAND_APPLY_SQRT, 0));

   for (size_t i = 0; i < 10; i++) {
      main_loop_clk();
   }
}

void Image_processing_simulation::send_mult(float value, bool clamp){
   uint8_t val_fixed_4_4 = 0;
   if( value < 0 ){
      val_fixed_4_4 = 0; //no sense to multiply an image by negative val
   }

   float value_buf = value;
   //run over 7 bits (4bits for signed decimal value, so 8th bit will be 0)
   for (int i = 6; i >= 0; i--) {
      if( value_buf >= pow(2, i-4) ) {
         value_buf -= pow(2, i-4);
         val_fixed_4_4 += 1<<i;
      }
   }

   // val_fixed_4_4 = 1<<4;

   fifo_in.push(Operation(true, COMMAND_APPLY_MULT, 0));
   fifo_in.push(Operation(false, COMMAND_NONE, val_fixed_4_4));
   fifo_in.push(Operation(false, COMMAND_NONE, clamp));

   for (size_t i = 0; i < 10; i++) {
      main_loop_clk();
   }
}

void Image_processing_simulation::wait_end_busy(){

   uint8_t status_out[4];

   do{
      fifo_in.push(Operation(true, COMMAND_GET_STATUS, 0));

      for (size_t i = 0; i < 100; i++) {
         main_loop_clk();
      }

      for (size_t i = 0; i < 4; i++) {
         status_out[i] = fifo_out.front();
         printf("status %lu = 0x%x\n", i, fifo_out.front());
         fifo_out.pop();
      }
      printf("wait_end_busy\n");
   } while( (status_out[0]&0x01 == 1) );
}

void Image_processing_simulation::read_image(uint8_t* image_out){

   fifo_in.push(Operation(true, COMMAND_READ_IMG, 0));

   for (size_t i = 0; i < image_width*image_height*20; i++) {
      main_loop_clk();
   }

   for (size_t i = 0; i < image_width*image_height; i++) {
      if(!fifo_out.empty()){
         image_out[i] = fifo_out.front();
         printf("read %d: 0x%x\n", i, image_out[i]);
         fifo_out.pop();
      }
   }
}

void Image_processing_simulation::switch_buffers(){
   fifo_in.push(Operation(true, COMMAND_SWITCH_BUFFERS, 0));

   for (size_t i = 0; i < 10; i++) {
      main_loop_clk();
   }
}


void Image_processing_simulation::send_binary_add(bool clamp){
   fifo_in.push(Operation(true, COMMAND_BINARY_ADD, 0));
   fifo_in.push(Operation(false, COMMAND_NONE, clamp));

   for (size_t i = 0; i < 100; i++) {
      main_loop_clk();
   }
}

void Image_processing_simulation::send_convolution(uint8_t *kernel, bool clamp, bool input_source, bool add_to_output){
   fifo_in.push(Operation(true, COMMAND_CONVOLUTION, 0));
   //parameters
   fifo_in.push(Operation(false, COMMAND_NONE, (add_to_output<<2)+(input_source<<1)+clamp));

   for (size_t i = 0; i < 9; i++) {
      fifo_in.push(Operation(false, COMMAND_NONE, kernel[i]));
   }

   for (size_t i = 0; i < 100; i++) {
      main_loop_clk();
   }
}

void Image_processing_simulation::send_clear(uint8_t value){
   this->send_threshold(0, value, true);
}



void Image_processing_simulation::main_loop_clk(){
   simulator->clk = 0;
   simulator->reset = 0;
   simulator->eval();
   simulator->clk = 1;
   simulator->reset = 0;
   simulator->comm_data_in_valid = 0;
   static int counter_free = 0;
   if(counter_free > 0){ //simulates the fact that the comm line can be full
      counter_free--;
   }
   simulator->comm_data_out_free = (counter_free == 0);
   if(!fifo_in.empty()){
      Operation op = fifo_in.front();
      fifo_in.pop();
      if(op.is_command){
         simulator->comm_data_in_valid = 1;
         simulator->comm_cmd = op.command;
      }
      else{
         simulator->comm_data_in_valid = 1;
         simulator->comm_data_in = op.data;
      }
   }

   if(simulator->comm_data_out_valid == 1){
      fifo_out.push(simulator->comm_data_out);
      counter_free = 3;
      simulator->comm_data_out_free = (counter_free == 0);
   }

   simulator->data_read_valid = 0;
   if(simulator->rd_en == 1){
      printf("read req addr: 0x%x\n", simulator->addr);
      simulator->data_read = memory[simulator->addr/2];
      simulator->data_read_valid = 1;
   }
   if(simulator->wr_en == 1){
      printf("wants to write: addr:0x%x data:0x%x\n", simulator->addr, simulator->data_write);
      memory[simulator->addr/2] = simulator->data_write;
   }

   simulator->eval();
}
