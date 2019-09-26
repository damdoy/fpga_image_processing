#include <stdint.h>
#include <queue>
#include <cmath>

#include "image_processing_ice40.hpp"

Image_processing_ice40::Image_processing_ice40(){

}

Image_processing_ice40::~Image_processing_ice40(){

}

void Image_processing_ice40::send_params(uint16_t img_width, uint16_t img_height){
   //
   // this->image_width = img_width;
   // this->image_height = img_height;
   //
   // uint8_t img_width8[2] = {img_width&0xFF, (img_width>>8)&0xFF};
   // uint8_t img_height8[2] = {img_height&0xFF, (img_height>>8)&0xFF};
   //
   // printf("sending img_width8[0]: %u, [1]: %u\n", img_width8[0], img_width8[1]);
   // printf("sending img_height8[0]: %u, [1]: %u\n", img_height8[0], img_height8[1]);
   //
   // fifo_in.push(Operation(true, COMMAND_PARAM, 0));
   // fifo_in.push(Operation(false, COMMAND_NONE, img_height8[0]));
   // fifo_in.push(Operation(false, COMMAND_NONE, img_height8[1]));
   // fifo_in.push(Operation(false, COMMAND_NONE, img_width8[0]));
   // fifo_in.push(Operation(false, COMMAND_NONE, img_width8[1]));
   //
   // for (size_t i = 0; i < 5; i++) {
   //    main_loop_clk();
   // }
}

void Image_processing_ice40::read_status(uint8_t *output){
   // fifo_in.push(Operation(true, COMMAND_GET_STATUS, 0));
   //
   // for (size_t i = 0; i < 6; i++) {
   //    main_loop_clk();
   // }
   //
   // while(!fifo_out.empty()){
   //    printf("read status: %u\n", fifo_out.front());
   //    fifo_out.pop();
   // }
   //
   // for (size_t i = 0; i < 4; i++) {
   //    if(!fifo_out.empty()){
   //       output[i] = fifo_out.front();
   //       fifo_out.pop();
   //    }
   // }
}

void Image_processing_ice40::send_image(uint8_t *image){
   // fifo_in.push(Operation(true, COMMAND_SEND_IMG, 0));
   // for (size_t i = 0; i < image_width*image_height; i++) {
   //    fifo_in.push(Operation(false, COMMAND_NONE, image[i]));
   // }
   //
   // for (size_t i = 0; i < image_width*image_height+500; i++) {
   //    main_loop_clk();
   // }
}

void Image_processing_ice40::send_add(int16_t value, bool clamp){
   // uint8_t add_value8[2] = {value&0xFF, (value>>8)&0xFF};
   //
   // fifo_in.push(Operation(true, COMMAND_APPLY_ADD, 0));
   // fifo_in.push(Operation(false, COMMAND_NONE, add_value8[0]));
   // fifo_in.push(Operation(false, COMMAND_NONE, add_value8[1]));
   // fifo_in.push(Operation(false, COMMAND_NONE, clamp));
   //
   // for (size_t i = 0; i < 100; i++) { //don't wait too long, will be checked with status
   //    main_loop_clk();
   // }
}

void Image_processing_ice40::send_threshold(uint8_t threshold_value, uint8_t replacement_value, bool upper_selection){
   // fifo_in.push(Operation(true, COMMAND_APPLY_THRESHOLD, 0));
   // fifo_in.push(Operation(false, COMMAND_NONE, threshold_value));
   // fifo_in.push(Operation(false, COMMAND_NONE, replacement_value));
   // fifo_in.push(Operation(false, COMMAND_NONE, upper_selection));
   //
   // for (size_t i = 0; i < 100; i++) {
   //    main_loop_clk();
   // }
}

void Image_processing_ice40::send_image_invert(){
   // fifo_in.push(Operation(true, COMMAND_APPLY_INVERT, 0));
   //
   // for (size_t i = 0; i < 10; i++) {
   //    main_loop_clk();
   // }
}

void Image_processing_ice40::send_pow(){
   // fifo_in.push(Operation(true, COMMAND_APPLY_POW, 0));
   //
   // for (size_t i = 0; i < 10; i++) {
   //    main_loop_clk();
   // }
}

void Image_processing_ice40::send_sqrt(){
   // fifo_in.push(Operation(true, COMMAND_APPLY_SQRT, 0));
   //
   // for (size_t i = 0; i < 10; i++) {
   //    main_loop_clk();
   // }
}

void Image_processing_ice40::send_mult(float value, bool clamp){
   // uint8_t val_fixed_4_4 = 0;
   // if( value < 0 ){
   //    val_fixed_4_4 = 0; //no sense to multiply an image by negative val
   // }
   //
   // float value_buf = value;
   // //run over 7 bits (4bits for signed decimal value, so 8th bit will be 0)
   // for (int i = 6; i >= 0; i--) {
   //    if( value_buf >= pow(2, i-4) ) {
   //       value_buf -= pow(2, i-4);
   //       val_fixed_4_4 += 1<<i;
   //    }
   // }
   //
   // // val_fixed_4_4 = 1<<4;
   //
   // fifo_in.push(Operation(true, COMMAND_APPLY_MULT, 0));
   // fifo_in.push(Operation(false, COMMAND_NONE, val_fixed_4_4));
   // fifo_in.push(Operation(false, COMMAND_NONE, clamp));
   //
   // for (size_t i = 0; i < 10; i++) {
   //    main_loop_clk();
   // }
}

void Image_processing_ice40::wait_end_busy(){

   // uint8_t status_out[4];
   //
   // do{
   //    fifo_in.push(Operation(true, COMMAND_GET_STATUS, 0));
   //
   //    for (size_t i = 0; i < 10; i++) {
   //       main_loop_clk();
   //    }
   //
   //    for (size_t i = 0; i < 4; i++) {
   //       status_out[i] = fifo_out.front();
   //       printf("status %lu = %d\n", i, fifo_out.front());
   //       fifo_out.pop();
   //    }
   //    printf("wait_end_busy\n");
   // } while( (status_out[0]&0x01 == 1) );
}

void Image_processing_ice40::read_image(uint8_t* image_out){

   // fifo_in.push(Operation(true, COMMAND_READ_IMG, 0));
   //
   // for (size_t i = 0; i < image_width*image_height*2; i++) {
   //    main_loop_clk();
   // }
   //
   // for (size_t i = 0; i < image_width*image_height; i++) {
   //    if(!fifo_out.empty()){
   //       image_out[i] = fifo_out.front();
   //       fifo_out.pop();
   //    }
   // }
}

void Image_processing_ice40::switch_buffers(){
   // fifo_in.push(Operation(true, COMMAND_SWITCH_BUFFERS, 0));
   //
   // for (size_t i = 0; i < 10; i++) {
   //    main_loop_clk();
   // }
}


void Image_processing_ice40::send_binary_add(bool clamp){
   // fifo_in.push(Operation(true, COMMAND_BINARY_ADD, 0));
   // fifo_in.push(Operation(false, COMMAND_NONE, clamp));
   //
   // for (size_t i = 0; i < 100; i++) {
   //    main_loop_clk();
   // }
}

void Image_processing_ice40::send_convolution(uint8_t *kernel, bool clamp, bool input_source, bool add_to_output){
   // fifo_in.push(Operation(true, COMMAND_CONVOLUTION, 0));
   // //parameters
   // fifo_in.push(Operation(false, COMMAND_NONE, (add_to_output<<2)+(input_source<<1)+clamp));
   //
   // for (size_t i = 0; i < 9; i++) {
   //    fifo_in.push(Operation(false, COMMAND_NONE, kernel[i]));
   // }
   //
   // for (size_t i = 0; i < 100; i++) {
   //    main_loop_clk();
   // }
}

void Image_processing_ice40::send_clear(uint8_t value){
   this->send_threshold(0, value, false);
}
