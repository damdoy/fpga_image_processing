#ifndef IMAGE_PROCESSING_H
#define IMAGE_PROCESSING_H

enum Commands {COMMAND_PARAM, COMMAND_SEND_IMG, COMMAND_READ_IMG, COMMAND_GET_STATUS, COMMAND_APPLY_ADD, COMMAND_APPLY_THRESHOLD,
               COMMAND_SWITCH_BUFFERS, COMMAND_BINARY_ADD, COMMAND_APPLY_INVERT,
               COMMAND_CONVOLUTION, COMMAND_BINARY_SUB, COMMAND_BINARY_MULT, COMMAND_APPLY_MULT, COMMAND_NONE=255};

class Image_processing {

public:
   Image_processing(){

   }

   virtual void send_params(uint16_t img_width, uint16_t img_height) = 0;
   virtual void read_status(uint8_t *output) = 0; //TODO vector?
   virtual void send_image(uint8_t *img) = 0;

   virtual void send_add(int16_t value, bool clamp) = 0;
   virtual void send_threshold(uint8_t threshold_value, uint8_t replacement, bool upper_selection) = 0;
   virtual void send_image_invert() = 0;
   virtual void send_mult(float value, bool clamp) = 0;

   virtual void wait_end_busy() = 0;

   virtual void read_image(uint8_t *img) = 0;

   virtual void switch_buffers() = 0;

   virtual void send_binary_add(bool clamp) = 0;
   virtual void send_binary_sub(bool clamp, bool absolute_diff) = 0;
   virtual void send_binary_mult(bool clamp) = 0;
   virtual void send_convolution(uint8_t *kernel, bool clamp, bool input_source, bool add_to_output) = 0;
   virtual void send_clear(uint8_t value) = 0;

protected:
   uint16_t image_width;
   uint16_t image_height;
};

#endif
