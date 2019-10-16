#include <stdint.h>
#include <queue>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "image_processing_ice40.hpp"

#define SPI_NOP 0x00
#define SPI_INIT 0x01
#define SPI_READ_DATA 0x02
#define SPI_SEND_CMD 0x03
#define SPI_SEND_DATA 0x04
#define SPI_READ_DATA16 0x05
#define SPI_SEND_DATA16 0x06

Image_processing_ice40::Image_processing_ice40(){

}

Image_processing_ice40::~Image_processing_ice40(){

}

void Image_processing_ice40::send_params(uint16_t img_width, uint16_t img_height){

   spi_init();

   uint8_t init_param[3] = {0x0, 0x0, 0x11};

   if (spi_command_send(SPI_INIT, init_param) != 0){
      printf("trouble to get answer\n");
   }

   this->image_width = img_width;
   this->image_height = img_height;

   uint8_t img_width8[2] = {img_width&0xFF, (img_width>>8)&0xFF};
   uint8_t img_height8[2] = {img_height&0xFF, (img_height>>8)&0xFF};
   // printf("ice40!\n");
   printf("sending img_width8[0]: %u, [1]: %u\n", img_width8[0], img_width8[1]);
   printf("sending img_height8[0]: %u, [1]: %u\n", img_height8[0], img_height8[1]);

   spi_command_send(SPI_SEND_CMD, COMMAND_PARAM);
   spi_command_send(SPI_SEND_DATA, img_width8[0]);
   spi_command_send(SPI_SEND_DATA, img_width8[1]);
   spi_command_send(SPI_SEND_DATA, img_height8[0]);
   spi_command_send(SPI_SEND_DATA, img_height8[1]);
}

void Image_processing_ice40::read_status(uint8_t *output){

   spi_command_send(SPI_SEND_CMD, COMMAND_GET_STATUS);

   uint8_t send_data[3]; //don't care
   uint8_t recv_data[2];
   int counter_output = 0;
   for (size_t i = 0; i < 100; i++) {
      spi_command_send_recv(SPI_READ_DATA, send_data, recv_data);
      printf("recv_data[0]: 0x%x\n", recv_data[0]);
      if(recv_data[0]&1 == 1 && counter_output<4){
         printf("recv data: 0x%x\n", recv_data[1]);
         output[counter_output] = recv_data[1];
         counter_output++;
      }
   }
}

void Image_processing_ice40::send_image(uint8_t *image){

   printf("sending..\n");
   spi_command_send(SPI_SEND_CMD, COMMAND_SEND_IMG);

   uint image_size = image_width*image_height;

   //fast send, 16 bytes per packet
   uint8_t data_to_send[16];
   size_t i = 0;
   for (i = 0; i < image_size; i+=16) {
      memcpy(data_to_send, image+i, 16);
      spi_command_send_16B(SPI_SEND_DATA16, data_to_send);
   }

   //padding if image is not a multiple of the fast send
   for (size_t j = 0; j < (image_size%16); j++) {
      spi_command_send(SPI_SEND_DATA, image[i+j]);
   }
}

void Image_processing_ice40::send_add(int16_t value, bool clamp){
   uint8_t add_value8[2] = {value&0xFF, (value>>8)&0xFF};

   spi_command_send(SPI_SEND_CMD, COMMAND_APPLY_ADD);
   spi_command_send(SPI_SEND_DATA, add_value8[0]);
   spi_command_send(SPI_SEND_DATA, add_value8[1]);
   spi_command_send(SPI_SEND_DATA, clamp);
}

void Image_processing_ice40::send_threshold(uint8_t threshold_value, uint8_t replacement_value, bool upper_selection){

   spi_command_send(SPI_SEND_CMD, COMMAND_APPLY_THRESHOLD);
   spi_command_send(SPI_SEND_DATA, threshold_value);
   spi_command_send(SPI_SEND_DATA, replacement_value);
   spi_command_send(SPI_SEND_DATA, upper_selection);
}

void Image_processing_ice40::send_image_invert(){

   spi_command_send(SPI_SEND_CMD, COMMAND_APPLY_INVERT);
}

void Image_processing_ice40::send_mult(float value, bool clamp){
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

   spi_command_send(SPI_SEND_CMD, COMMAND_APPLY_MULT);
   spi_command_send(SPI_SEND_DATA, val_fixed_4_4);
   spi_command_send(SPI_SEND_DATA, clamp);
}

void Image_processing_ice40::wait_end_busy(){

   uint8_t status_out[4];

   do{
      read_status(status_out);
      printf("wait_end_busy\n");
   } while( (status_out[0]&0x01 == 1) );
}

void Image_processing_ice40::read_image(uint8_t* image_out){

   spi_command_send(SPI_SEND_CMD, COMMAND_READ_IMG);

   uint8_t send_data[16]; //don't care
   uint8_t recv_data[15];
   int counter_read = 0;
   while(counter_read < image_width*image_height) {
      spi_command_send_recv_16B(SPI_READ_DATA16, send_data, recv_data);
      printf("recv_data[0] (status): 0x%x\n", recv_data[0]);
      if(recv_data[0]&1 == 1){
         for (size_t i = 1; i < 15; i++) {
            //if we ask more pixels that the size of image (image not divisible by 15) then we will have garbage which we don't care
            if(counter_read < image_width*image_height){
               printf("recv data[%lu]: 0x%x\n", i,recv_data[i]);
               image_out[counter_read] = recv_data[i];
               counter_read++;
            }
         }
      }
   }
   // spi_command_send(SPI_SEND_CMD, COMMAND_READ_IMG);
   //
   // uint8_t send_data[3]; //don't care
   // uint8_t recv_data[2];
   // int counter_read = 0;
   // while(counter_read < image_width*image_height) {
   //    spi_command_send_recv(SPI_READ_DATA, send_data, recv_data);
   //    printf("recv_data[0]: 0x%x\n", recv_data[0]);
   //    if(recv_data[0]&1 == 1 && counter_read < image_width*image_height){
   //       printf("recv data: 0x%x\n", recv_data[1]);
   //       image_out[counter_read] = recv_data[1];
   //       counter_read++;
   //    }
   // }
}

void Image_processing_ice40::switch_buffers(){
   spi_command_send(SPI_SEND_CMD, COMMAND_SWITCH_BUFFERS);
}


void Image_processing_ice40::send_binary_add(bool clamp){
   spi_command_send(SPI_SEND_CMD, COMMAND_BINARY_ADD);
   spi_command_send(SPI_SEND_DATA, clamp);
}

void Image_processing_ice40::send_binary_sub(bool clamp, bool absolute_diff){
   spi_command_send(SPI_SEND_CMD, COMMAND_BINARY_SUB);
   spi_command_send(SPI_SEND_DATA, (absolute_diff<<1)+clamp);
}

void Image_processing_ice40::send_binary_mult(bool clamp){
   spi_command_send(SPI_SEND_CMD, COMMAND_BINARY_MULT);
   spi_command_send(SPI_SEND_DATA, clamp);
}


void Image_processing_ice40::send_convolution(uint8_t *kernel, bool clamp, bool input_source, bool add_to_output){
   spi_command_send(SPI_SEND_CMD, COMMAND_CONVOLUTION);
   spi_command_send(SPI_SEND_DATA, (add_to_output<<2)+(input_source<<1)+clamp);

   for (size_t i = 0; i < 9; i++) {
      spi_command_send(SPI_SEND_DATA, kernel[i]);
   }
}

void Image_processing_ice40::send_clear(uint8_t value){
   this->send_threshold(0, value, true);
}
