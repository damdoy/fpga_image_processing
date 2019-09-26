#include <cstdio>
#include <queue>
#include <stdint.h>

// #include "images/image_fruits_128.h"
#include "images/image_fruits_64.h"
// #include "images/image_fruits_8.h"
// #include "images/image_sequential.h"

#include "image_processing.hpp"

#ifdef SIMULATION
#include "../simulation/image_processing_simulation.hpp"
#elif ICE40
#include "../ice40/software/image_processing_ice40.hpp"
#endif

void test_add_threshold(uint8_t *image_input, uint8_t *image_output, Image_processing *img_proc){
   img_proc->send_params(image_width, image_height);

   uint8_t status[4];
   img_proc->read_status(status);
   printf("after send params\n");
   for (size_t i = 0; i < 4; i++) {
      printf("status out %lu : 0x%x\n", i, status[i]);
   }

   img_proc->send_image(image_input);

   printf("===========ADD===========\n");
   img_proc->switch_buffers();
   img_proc->send_add(32, true);
   img_proc->read_status(status);
   printf("after ADD\n");
   for (size_t i = 0; i < 4; i++) {
      printf("status out %lu : 0x%x\n", i, status[i]);
   }

   img_proc->wait_end_busy();

   img_proc->read_status(status);
   printf("Finished ADD\n");
   for (size_t i = 0; i < 4; i++) {
      printf("status out %lu : 0x%x\n", i, status[i]);
   }

   printf("===========THRESHOLD===========\n");
   img_proc->send_threshold(168, 0, 0);
   img_proc->wait_end_busy();

   img_proc->switch_buffers();
   img_proc->read_image(image_output);
}

//will load the image in the input buffer and set the storage buffer to pixels of value 32
// and then will add the two buffers
void test_binary_add(uint8_t *image_input, uint8_t *image_output, Image_processing *img_proc){
   img_proc->send_params(image_width, image_height);
   img_proc->send_image(image_input); //in input buffer

   img_proc->send_clear(32); //storage buffer will have an image full of pixels 32
   img_proc->wait_end_busy();

   img_proc->switch_buffers();

   img_proc->send_binary_add(true);
   img_proc->wait_end_busy();

   img_proc->switch_buffers();
   img_proc->read_image(image_output);
}

void test_gaussian_blur(uint8_t *image_input, uint8_t *image_output, Image_processing *img_proc){
   img_proc->send_params(image_width, image_height);
   img_proc->send_image(image_input);

   //gaussian blur kernel
   uint8_t conv_kernel[9] = {(1)<<0, (1)<<1, (1)<<0, (1)<<1, ((1)<<2), (1)<<1,  (1)<<0, (1)<<1, (1)<<0};

   // uint8_t conv_kernel[9] = {(1)<<4, (1)<<4, (1)<<4, (1)<<4, ((-7)<<4), (1)<<4,  (1)<<4, (1)<<4, (1)<<4};

   img_proc->send_convolution(conv_kernel, true, true, false);

   printf("wait end busy\n");

   img_proc->wait_end_busy();

   printf("switch buffers\n");

   img_proc->switch_buffers();

   img_proc->read_image(image_output);
}

//use gradient kernels
void test_simple_edge_detection(uint8_t *image_input, uint8_t *image_output, Image_processing *img_proc){
   img_proc->send_params(image_width, image_height);
   img_proc->send_image(image_input);

   //top gradient
   {
      uint8_t conv_kernel[9] = {(1)<<3, (1)<<4, (1)<<3, (0)<<4, ((0)<<4), (0)<<4,  (-1)<<3, (-1)<<4, (-1)<<3};
      img_proc->send_convolution(conv_kernel, true, true, true);
   }
   img_proc->wait_end_busy();

   //bottom gradient
   {
      uint8_t conv_kernel[9] = {(-1)<<3, (-1)<<4, (-1)<<3, (0)<<4, ((0)<<4), (0)<<4,  (1)<<3, (1)<<4, (1)<<3};
      img_proc->send_convolution(conv_kernel, true, true, true);
   }
   img_proc->wait_end_busy();

   //left gradient
   {
      uint8_t conv_kernel[9] = {(1)<<3, (0)<<4, (-1)<<3, (1)<<4, ((0)<<4), (-1)<<4,  (1)<<3, (0)<<4, (-1)<<3};
      img_proc->send_convolution(conv_kernel, true, true, true);
   }
   img_proc->wait_end_busy();

   //right gradient
   {
      uint8_t conv_kernel[9] = {(-1)<<3, (0)<<4, (1)<<3, (-1)<<4, ((0)<<4), (1)<<4,  (-1)<<3, (0)<<4, (1)<<3};
      img_proc->send_convolution(conv_kernel, true, true, true);
   }
   img_proc->wait_end_busy();

   img_proc->switch_buffers();

   img_proc->read_image(image_output);
}

void test_multiplication(uint8_t *image_input, uint8_t *image_output, Image_processing *img_proc){

   img_proc->send_params(image_width, image_height);
   img_proc->send_image(image_input);

   img_proc->switch_buffers();

   img_proc->send_mult(0.5f, true);
   img_proc->wait_end_busy();

   img_proc->switch_buffers();
   img_proc->read_image(image_output);
}

int main(){
   FILE *output_file = fopen("output.dat", "w");

   Image_processing *img_proc;

   #ifdef SIMULATION
   img_proc = new Image_processing_simulation();
   #elif ICE40
   img_proc = new Image_processing_ice40();
   #endif

   uint8_t *image_input = new uint8_t[image_width*image_height];
   uint8_t *image_output = new uint8_t[image_width*image_height];

   const char *ptr_image = header_data;
   for (size_t i = 0; i < image_height*image_width; i++) {
      uint8_t pixel[3];
      HEADER_PIXEL(ptr_image, pixel);
      image_input[i] = pixel[0];
   }

   //test selection
   // test_add_threshold(image_input, image_output, img_proc);
   // test_binary_add(image_input, image_output, img_proc);
   // test_gaussian_blur(image_input, image_output, img_proc);
   test_simple_edge_detection(image_input, image_output, img_proc);
   // test_multiplication(image_input, image_output, img_proc);

   // test_simulation(image_input, image_output);

   for (size_t i = 0; i < image_height*image_width; i++) {
      fprintf(output_file, "%d ", image_output[i]);
      if( ((i+1) % (image_width)) == 0){
         fprintf(output_file, "\n");
      }
   }

   fclose(output_file);

   delete img_proc;
   return 0;
}
