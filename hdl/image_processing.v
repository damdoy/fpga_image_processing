/* verilator lint_off UNUSED */

module image_processing(
clk, reset,

//memory access
addr, wr_en, rd_en, data_read, data_read_valid, data_write,

//comm module access
comm_cmd, comm_data_in, comm_data_in_valid, comm_data_out, comm_data_out_valid, comm_data_out_free
);

input wire clk;
input wire reset;

//memory interface
output reg [31:0] addr;
output reg wr_en;
output reg rd_en;
output reg [15:0] data_write;
input wire [15:0] data_read;
input wire data_read_valid;

//comm module interface
input wire [7:0] comm_cmd;
input wire [7:0] comm_data_in;
input wire comm_data_in_valid;
output reg [7:0] comm_data_out;
output reg comm_data_out_valid;
input wire comm_data_out_free;

parameter STATE_IDLE = 0, STATE_WAIT_COMMAND = STATE_IDLE+1, STATE_READ_COMMAND_PARAM_WIDTH=STATE_WAIT_COMMAND+1,
          STATE_READ_COMMAND_PARAM_HEIGHT = STATE_READ_COMMAND_PARAM_WIDTH+1, STATE_SEND_IMG = STATE_READ_COMMAND_PARAM_HEIGHT+1,
          STATE_READ_IMG = STATE_SEND_IMG+1, STATE_GET_STATUS = STATE_READ_IMG+1,
          STATE_APPLY_ADD_READ_PARAM = STATE_GET_STATUS+1, STATE_APPLY_ADD = STATE_APPLY_ADD_READ_PARAM+1,
          STATE_THRESHOLD_READ_PARAM = STATE_APPLY_ADD+1, STATE_PROC_THRESHOLD = STATE_THRESHOLD_READ_PARAM+1,
          STATE_BINARY_ADD_READ_PARAM = STATE_PROC_THRESHOLD+1, STATE_PROC_BINARY = STATE_BINARY_ADD_READ_PARAM+1,
          STATE_APPLY_INVERT_READ_PARAM = STATE_PROC_BINARY+1, STATE_PROC_UNARY = STATE_APPLY_INVERT_READ_PARAM+1,
          STATE_APPLY_POW_READ_PARAM = STATE_PROC_UNARY+1,
          STATE_CONVOLUTION_READ_PARAM = STATE_APPLY_POW_READ_PARAM+1, STATE_PROC_CONVOLUTION = STATE_CONVOLUTION_READ_PARAM+1,
          STATE_BINARY_SUB_READ_PARAM = STATE_PROC_CONVOLUTION+1, STATE_BINARY_MULT_READ_PARAM = STATE_BINARY_SUB_READ_PARAM+1,
          STATE_APPLY_MULT_READ_PARAM = STATE_BINARY_MULT_READ_PARAM+1, STATE_APPLY_MULT = STATE_APPLY_MULT_READ_PARAM+1;

//only works in systemverilog
// enum bit [7:0] {STATE_IDLE, STATE_WAIT_COMMAND, STATE_READ_COMMAND_PARAM_WIDTH,
//           STATE_READ_COMMAND_PARAM_HEIGHT, STATE_SEND_IMG,
//           STATE_READ_IMG, STATE_GET_STATUS, STATE_APPLY_ADD_READ_PARAM, STATE_APPLY_ADD,
//           STATE_THRESHOLD_READ_PARAM, STATE_PROC_THRESHOLD,
//           STATE_BINARY_ADD_READ_PARAM, STATE_PROC_BINARY,
//           STATE_APPLY_INVERT_READ_PARAM, STATE_PROC_UNARY,
//           STATE_APPLY_POW_READ_PARAM,
//           STATE_CONVOLUTION_READ_PARAM, STATE_PROC_CONVOLUTION,
//           STATE_BINARY_SUB_READ_PARAM, STATE_BINARY_MULT_READ_PARAM,
//           STATE_APPLY_MULT_READ_PARAM, STATE_APPLY_MULT} States;

reg [7:0] state;

reg [7:0] state_processing;
reg [7:0] processing_command;


parameter COMMAND_PARAM = 0, COMMAND_SEND_IMG = COMMAND_PARAM+1, COMMAND_READ_IMG = COMMAND_SEND_IMG+1,
          COMMAND_GET_STATUS = COMMAND_READ_IMG+1, COMMAND_APPLY_ADD = COMMAND_GET_STATUS+1, COMMAND_APPLY_THRESHOLD = COMMAND_APPLY_ADD+1,
          COMMAND_SWITCH_BUFFERS = COMMAND_APPLY_THRESHOLD+1, COMMAND_BINARY_ADD = COMMAND_SWITCH_BUFFERS+1,
          COMMAND_APPLY_INVERT = COMMAND_BINARY_ADD+1, COMMAND_APPLY_POW = COMMAND_APPLY_INVERT+1, COMMAND_APPLY_SQRT = COMMAND_APPLY_POW+1,
          COMMAND_CONVOLUTION = COMMAND_APPLY_SQRT+1, COMMAND_BINARY_SUB = COMMAND_CONVOLUTION+1, COMMAND_BINARY_MULT = COMMAND_BINARY_SUB+1,
          COMMAND_APPLY_MULT = COMMAND_BINARY_MULT+1;

//systemverilog enum
// enum bit [7:0] {
//    COMMAND_PARAM, COMMAND_SEND_IMG, COMMAND_READ_IMG,
//    COMMAND_GET_STATUS, COMMAND_APPLY_ADD, COMMAND_APPLY_THRESHOLD,COMMAND_SWITCH_BUFFERS, COMMAND_BINARY_ADD,
//    COMMAND_APPLY_INVERT, COMMAND_APPLY_POW, COMMAND_APPLY_SQRT,COMMAND_CONVOLUTION, COMMAND_BINARY_SUB, COMMAND_BINARY_MULT,
// COMMAND_APPLY_MULT
// } Commands;

parameter MEMORY_SIZE = 1024*128;
parameter BUFFER_SIZE = MEMORY_SIZE/2;
parameter BUFFER2_LOCATION = MEMORY_SIZE/2;

parameter CONVOLUTION_LINE_MAX_SIZE = 256;

////////local reg
reg [15:0] counter_read;
reg [31:0] memory_addr_counter;
reg [15:0] mem_data_buffer;
reg mem_data_buffer_full;
reg [15:0] buffer_read;

reg [15:0] proc_counter_read;
reg [31:0] proc_memory_addr_counter;
reg [31:0] proc_conv_memory_addr_read;
reg [31:0] proc_conv_memory_addr_write;

reg binary_read_buffer;

//will keep the previous lines for convolution
reg [7:0] convolution_buffer [0:CONVOLUTION_LINE_MAX_SIZE-1][0:1];

reg [15:0] counter_convolution_x;
reg [15:0] counter_convolution_y;

//since we have to read in advance, there is a slight offset between the read and write
reg [15:0] counter_convolution_x_write;
reg [15:0] counter_convolution_y_write;

//buffers to keep last reads, as we need 3x3 matrices but only have two lines of buffers
reg [15:0] convolution_previous_read[1:0];
reg [15:0] convolution_previous_read_counter_x[1:0];
reg [15:0] convolution_previous_read_counter_y[1:0];
reg [7:0] convolution_last_calculation;

reg [1:0] convolution_reading_data;
reg [15:0] convolution_data_to_add;

//params
reg [15:0] img_width;
reg [15:0] img_height;

reg [15:0] add_value;
reg clamp;
reg convolution_source_input;
reg convolution_add_to_result;

reg [7:0] threshold_value;
reg [7:0] threshold_replacement;
reg threshold_upper;

reg [31:0] buffer_input_address;
reg [31:0] buffer_storage_address;

reg [15:0] convolution_matrix [0:8]; //3x3 matrix
reg [7:0] convolution_params;

reg [7:0] mult_value_param;

//returns 8bit value from 16bit with possible clamping
function [7:0] apply_clamp;
input [15:0] in;
input clamp_en;
begin
   apply_clamp = in[7:0];
   if(clamp_en == 1 && $signed(in) > 255)begin
      apply_clamp = 255;
   end
   if(clamp_en == 1 && $signed(in) < 0) begin
      apply_clamp = 0;
   end
end
endfunction

//same as apply_clamp but with a fixed point value
function [7:0] apply_clamp_fixed16;
input [15:0] in;
input clamp_en;
begin
   apply_clamp_fixed16 = in[11:4];
   if(clamp_en == 1 && $signed(in[15:4]) > 255)begin
      apply_clamp_fixed16 = 255;
   end
   if(clamp_en == 1 && $signed(in[15:4]) < 0) begin
      apply_clamp_fixed16 = 0;
   end
end
endfunction

initial begin
   //outputs reg
   addr = 32'b0;
   wr_en = 0;
   rd_en = 0;
   data_write = 16'b0;

   mem_data_buffer_full = 0;
   comm_data_out = 8'b0;
   comm_data_out_valid = 0;

   proc_counter_read = 0;
   proc_memory_addr_counter = 0;

   binary_read_buffer = 0;

   state = STATE_WAIT_COMMAND;
   state_processing = STATE_IDLE;

   counter_read = 0;

   buffer_input_address = 0;
   //doesn't want to be init at this value ==> do it in init state
   // buffer_storage_address = BUFFER2_LOCATION;
   buffer_storage_address = 0;

   img_width = 0;
   img_height = 0;
end

always @(posedge clk)
begin

   //default
   comm_data_out_valid <= 0;
   wr_en <= 0;
   rd_en <= 0;

   case (state)
   STATE_IDLE: begin
   end
   STATE_WAIT_COMMAND: begin
      if(comm_data_in_valid == 1)
      begin
         case (comm_cmd)
         COMMAND_PARAM: begin
            state <= STATE_READ_COMMAND_PARAM_WIDTH;
            counter_read <= 1; //will be used to read the 16bits
            buffer_storage_address <= BUFFER2_LOCATION;
            buffer_input_address <= 0;
         end
         COMMAND_SEND_IMG: begin
            state <= STATE_SEND_IMG;
            counter_read <= img_width*img_height;
            memory_addr_counter <= buffer_input_address;
         end
         COMMAND_READ_IMG: begin
            state <= STATE_READ_IMG;
            counter_read <= img_width*img_height;
            memory_addr_counter <= buffer_input_address;
         end
         COMMAND_GET_STATUS: begin
            state <= STATE_GET_STATUS;
            counter_read <= 3;
         end
         COMMAND_APPLY_ADD: begin
            state <= STATE_APPLY_ADD_READ_PARAM;
            counter_read <= 2; //read 16bit of parameters
         end
         COMMAND_APPLY_THRESHOLD: begin
            state <= STATE_THRESHOLD_READ_PARAM;
            counter_read <= 2; // read 3*8bits of params
         end
         COMMAND_SWITCH_BUFFERS: begin
            state <= STATE_WAIT_COMMAND;
            buffer_input_address <= buffer_storage_address;
            buffer_storage_address <= buffer_input_address;
         end
         COMMAND_BINARY_ADD: begin
            state <= STATE_BINARY_ADD_READ_PARAM;
            counter_read <= 0;
         end
         COMMAND_APPLY_INVERT: begin
            state <= STATE_WAIT_COMMAND;
            state_processing <= STATE_PROC_UNARY;
            processing_command <= COMMAND_APPLY_INVERT;
            counter_read <= 0;
            proc_counter_read <= img_width*img_height;
            proc_memory_addr_counter <= buffer_storage_address;
         end
         COMMAND_APPLY_POW: begin
            state <= STATE_APPLY_POW_READ_PARAM;
            counter_read <= 0;
         end
         COMMAND_APPLY_SQRT: begin
            state <= STATE_WAIT_COMMAND;
            state_processing <= STATE_PROC_UNARY;
            processing_command <= COMMAND_APPLY_SQRT;
            counter_read <= 0;
            proc_counter_read <= img_width*img_height;
            proc_memory_addr_counter <= buffer_storage_address;
         end
         COMMAND_CONVOLUTION: begin
            state <= STATE_CONVOLUTION_READ_PARAM;
            counter_read <= 9;
         end
         COMMAND_BINARY_SUB: begin
            state <= STATE_BINARY_ADD_READ_PARAM;
            counter_read <= 0;
         end
         COMMAND_APPLY_MULT: begin
            state <= STATE_APPLY_MULT_READ_PARAM;
            counter_read <= 1;
         end
         default: begin
         end
         endcase
      end
   end
   STATE_READ_COMMAND_PARAM_WIDTH: begin
      if(comm_data_in_valid == 1)begin
         if(counter_read == 1) begin
            img_width[7:0] <= comm_data_in; //TODO: verify endianenes
            counter_read <= 0;
         end else begin
            img_width[15:8] <= comm_data_in; //TODO: verify endianenes
            state <= STATE_READ_COMMAND_PARAM_HEIGHT;
            counter_read <= 1;
         end
      end
   end
   STATE_READ_COMMAND_PARAM_HEIGHT: begin
      if(comm_data_in_valid == 1)begin
         if(counter_read == 1) begin
            img_height[7:0] <= comm_data_in; //TODO: verify endianenes
            counter_read <= 0;
         end else begin
            img_height[15:8] <= comm_data_in; //TODO: verify endianenes
            state <= STATE_WAIT_COMMAND; //just wait next command
         end
      end
   end
   STATE_GET_STATUS: begin
      if(comm_data_out_free == 1) begin
         if(counter_read == 3) begin
            comm_data_out_valid <= 1;
            comm_data_out[7:0] <= 8'h11;
            comm_data_out[0] <= ~(state_processing == STATE_IDLE);
         end else if(counter_read == 2) begin
            comm_data_out_valid <= 1;
            comm_data_out[7:0] <= 8'h22;
         end else if(counter_read == 1) begin
            comm_data_out_valid <= 1;
            comm_data_out[7:0] <= 8'h33;
         end else begin
            comm_data_out_valid <= 1;
            comm_data_out[7:0] <= 8'h44;
         end

         if(counter_read > 0) begin
            counter_read <= counter_read - 1;
         end else begin //counter_read == 0
            state <= STATE_WAIT_COMMAND;
         end
      end
   end
   STATE_SEND_IMG: begin
      if(comm_data_in_valid == 1) begin

         if(memory_addr_counter[0] == 1'b0) begin
            data_write[7:0] <= comm_data_in;
         end else begin
            data_write[15:8] <= comm_data_in;
            wr_en <= 1;
            addr <= {memory_addr_counter[31:1], 1'b0};
         end

         memory_addr_counter <= memory_addr_counter+1;

         if(counter_read > 1) begin
            counter_read <= counter_read - 1;
         end else begin
            state <= STATE_WAIT_COMMAND;
         end
      end
   end
   STATE_APPLY_ADD_READ_PARAM: begin
      if(comm_data_in_valid == 1)begin
         if(counter_read == 2) begin
            add_value[7:0] <= comm_data_in;
            counter_read <= 1;
         end else if(counter_read == 1) begin
            add_value[15:8] <= comm_data_in;
            counter_read <= 0;
         end else begin
            clamp <= comm_data_in[0];
            state_processing <= STATE_PROC_UNARY;
            processing_command <= COMMAND_APPLY_ADD;
            state <= STATE_WAIT_COMMAND;
            proc_counter_read <= img_width*img_height;
            // proc_counter_read <= 20;
            // memory_addr_counter <= 0;
            proc_memory_addr_counter <= buffer_storage_address;
         end
      end
   end
   STATE_READ_IMG: begin
      if(memory_addr_counter[0] == 1'b0) begin
         rd_en <= 1;
         addr <= memory_addr_counter;
         memory_addr_counter <= memory_addr_counter + 1;
      end else begin
         if (counter_read[0] == 1'b0 && data_read_valid == 1'b1) begin //image size mod 2 should be 0
            mem_data_buffer <= data_read;
            mem_data_buffer_full <= 1;
         end

         if( comm_data_out_free == 1 && mem_data_buffer_full == 1 ) begin
            if (counter_read[0] == 1'b0) begin //image size mod 2 should be 0
               comm_data_out_valid <= 1;
               comm_data_out <= data_read[7:0];
               mem_data_buffer <= data_read;
               counter_read <= counter_read - 1;
            end else begin

               comm_data_out_valid <= 1;
               comm_data_out <= mem_data_buffer[15:8];
               counter_read <= counter_read - 1;
               memory_addr_counter <= memory_addr_counter+1;
               mem_data_buffer_full <= 0;
               if(counter_read <= 1) begin // > 1 and not 0 because we are shifted by one
                  state <= STATE_WAIT_COMMAND;
               end
            end
         end
      end
   end
   STATE_THRESHOLD_READ_PARAM: begin
      if(comm_data_in_valid == 1)begin
         if(counter_read == 2) begin
            threshold_value[7:0] <= comm_data_in;
            counter_read <= 1;
         end else if(counter_read == 1) begin
            threshold_replacement[7:0] <= comm_data_in;
            counter_read <= 0;
         end else begin
            threshold_upper <= comm_data_in[0];
            state_processing <= STATE_PROC_UNARY;
            processing_command <= COMMAND_APPLY_THRESHOLD;
            state <= STATE_WAIT_COMMAND;
            proc_counter_read <= img_width*img_height;
            proc_memory_addr_counter <= buffer_storage_address;
         end
      end
   end
   STATE_BINARY_ADD_READ_PARAM: begin
      if(comm_data_in_valid == 1)begin
         state_processing <= STATE_PROC_BINARY;
         processing_command <= COMMAND_BINARY_ADD;
         state <= STATE_WAIT_COMMAND;
         clamp <= comm_data_in[0];
         proc_counter_read <= img_width*img_height;
         proc_memory_addr_counter <= 0; //offset
         binary_read_buffer <= 0;
      end
   end
   STATE_APPLY_POW_READ_PARAM: begin
      if(comm_data_in_valid == 1)begin
         state_processing <= STATE_PROC_UNARY;
         processing_command <= COMMAND_APPLY_POW;
         state <= STATE_WAIT_COMMAND;
         clamp <= comm_data_in[0];
         proc_counter_read <= img_width*img_height;
         proc_memory_addr_counter <= 0; //offset
      end
   end
   STATE_CONVOLUTION_READ_PARAM: begin
      if(comm_data_in_valid == 1) begin
         if(counter_read == 9)begin //first value are gneral params
            convolution_params <= comm_data_in;
            clamp <= comm_data_in[0];
            convolution_source_input <= comm_data_in[1];
            convolution_add_to_result <= comm_data_in[2];
            counter_read <= 8;
         end else begin //the kernel matrix (3x3)
            //sign extend the 8bits values into 16bits
            convolution_matrix[8-counter_read] <= { {8{comm_data_in[7]}}, comm_data_in};
            counter_read <= counter_read - 1;

            if(counter_read == 0) begin
               state_processing <= STATE_PROC_CONVOLUTION;
               state <= STATE_WAIT_COMMAND;
               proc_counter_read <= img_width*img_height;
               counter_convolution_x <= 0;
               counter_convolution_y <= 0;
               counter_convolution_x_write <= 0;
               counter_convolution_y_write <= 0;
               convolution_reading_data <= 2'b00;
               if(convolution_source_input == 1) begin //input buffer used as source for convolution
                  proc_conv_memory_addr_read <= buffer_input_address;
               end else begin
                  proc_conv_memory_addr_read <= buffer_storage_address;
               end
               proc_conv_memory_addr_write <= buffer_storage_address;
            end
         end
      end
   end
   STATE_BINARY_SUB_READ_PARAM: begin
      if(comm_data_in_valid == 1)begin
         state_processing <= STATE_PROC_BINARY;
         processing_command <= COMMAND_BINARY_SUB;
         state <= STATE_WAIT_COMMAND;
         clamp <= comm_data_in[0];
         proc_counter_read <= img_width*img_height;
         proc_memory_addr_counter <= 0; //offset
         binary_read_buffer <= 0;
      end
   end
   STATE_BINARY_MULT_READ_PARAM: begin
      if(comm_data_in_valid == 1)begin
         state_processing <= STATE_PROC_BINARY;
         processing_command <= COMMAND_BINARY_MULT;
         state <= STATE_WAIT_COMMAND;
         clamp <= comm_data_in[0];
         proc_counter_read <= img_width*img_height;
         proc_memory_addr_counter <= 0; //offset
         binary_read_buffer <= 0;
      end
   end
   STATE_APPLY_MULT_READ_PARAM: begin
      if(comm_data_in_valid == 1 && counter_read == 1) begin
         mult_value_param <= comm_data_in;
         counter_read <= 0;
      end else if (comm_data_in_valid == 1 && counter_read == 0) begin
         state_processing <= STATE_PROC_UNARY;
         processing_command <= COMMAND_APPLY_MULT;
         state <= STATE_WAIT_COMMAND;
         clamp <= comm_data_in[0];
         proc_counter_read <= img_width*img_height;
         proc_memory_addr_counter <= 0; //offset
      end
   end
   default: begin
   end
   endcase

   case (state_processing)
   STATE_IDLE: begin
   end
   STATE_PROC_UNARY: begin : unary
      reg [15:0] temp_calc; //used for calculations
      //assume single port ram
      if(proc_memory_addr_counter[0] == 1'b0) begin
         rd_en <= 1;
         addr <= proc_memory_addr_counter;
         proc_memory_addr_counter <= proc_memory_addr_counter+1;
      end else begin
         if (data_read_valid == 1'b1) begin
            wr_en <= 1;
            addr <= {proc_memory_addr_counter[31:1], 1'b0};

            if(processing_command == COMMAND_APPLY_ADD)begin
               temp_calc = {8'b0, data_read[7:0]}+add_value;
               data_write[7:0] <= apply_clamp(temp_calc, clamp);

               temp_calc = {8'b0, data_read[15:8]}+add_value;
               data_write[15:8] <= apply_clamp(temp_calc, clamp);
            end else if(processing_command == COMMAND_APPLY_THRESHOLD) begin
               data_write <= data_read;
               if(threshold_upper == 1) begin
                  if(data_read[7:0] >= threshold_value) begin
                     data_write[7:0] <= threshold_replacement;
                  end
                  if(data_read[15:8] >= threshold_value) begin
                     data_write[15:8] <= threshold_replacement;
                  end
               end else begin
                  if(data_read[7:0] <= threshold_value) begin
                     data_write[7:0] <= threshold_replacement;
                  end
                  if(data_read[15:8] <= threshold_value) begin
                     data_write[15:8] <= threshold_replacement;
                  end
               end
            end else if(processing_command == COMMAND_APPLY_INVERT) begin
               data_write <= ~data_read;
            end else if(processing_command == COMMAND_APPLY_POW) begin
               temp_calc = {7'b0, data_read[7:0], 1'b0}; //TODO: this is not a pow
               data_write[7:0] <= apply_clamp(temp_calc, clamp);

               temp_calc = {7'b0, data_read[15:8], 1'b0};
               data_write[15:8] <= apply_clamp(temp_calc, clamp);
            end else if(processing_command == COMMAND_APPLY_SQRT) begin
               data_write[7:0] <= {1'b0, data_read[7:1]}; //TODO: this is not a sqrt
               data_write[15:8] <= {1'b0, data_read[15:9]};
            end else if(processing_command == COMMAND_APPLY_MULT) begin
               temp_calc = mult_value_param*data_read[7:0];
               data_write[7:0] <= apply_clamp_fixed16(temp_calc, clamp);
               temp_calc = mult_value_param*data_read[15:8];
               data_write[15:8] <= apply_clamp_fixed16(temp_calc, clamp);
            end

            proc_memory_addr_counter <= proc_memory_addr_counter+1;
            if(proc_counter_read > 2) begin // > 2 and not 0 because we are shifted by one
               proc_counter_read <= proc_counter_read - 2;
            end else begin
               state_processing <= STATE_IDLE;
            end
         end
      end
   end
   STATE_PROC_BINARY: begin : binary
      reg [15:0] temp_calc; //used for calculations
      //assume single port ram
      if(proc_memory_addr_counter[0] == 1'b0 && binary_read_buffer == 0) begin
         rd_en <= 1;
         addr <= buffer_input_address+proc_memory_addr_counter;
         binary_read_buffer <= 1;
      end if (proc_memory_addr_counter[0] == 1'b0 && binary_read_buffer == 1) begin
         if (data_read_valid == 1'b1) begin
            rd_en <= 1;
            addr <= buffer_storage_address+proc_memory_addr_counter;
            buffer_read <= data_read;
            binary_read_buffer <= 0;
            proc_memory_addr_counter <= proc_memory_addr_counter + 1;
         end
      end else begin
         if (data_read_valid == 1'b1) begin
            wr_en <= 1;
            addr <= buffer_storage_address+{proc_memory_addr_counter[31:1], 1'b0};

            if( processing_command == COMMAND_BINARY_ADD) begin
               temp_calc = {8'b0, buffer_read[7:0]} + {8'b0, data_read[7:0]};
               data_write[7:0] <= apply_clamp(temp_calc, clamp);
               temp_calc = {8'b0, buffer_read[15:8]} + {8'b0, data_read[15:8]};
               data_write[15:8] <= apply_clamp(temp_calc, clamp);
            end else if (processing_command == COMMAND_BINARY_SUB) begin
               temp_calc = {8'b0, buffer_read[7:0]} - {8'b0, data_read[7:0]};
               data_write[7:0] <= apply_clamp(temp_calc, clamp);
               temp_calc = {8'b0, buffer_read[15:8]} - {8'b0, data_read[15:8]};
               data_write[15:8] <= apply_clamp(temp_calc, clamp);
            end else if (processing_command == COMMAND_BINARY_MULT) begin
               temp_calc = {8'b0, buffer_read[7:0]} * {8'b0, data_read[7:0]};
               data_write[7:0] <= apply_clamp(temp_calc, clamp);
               temp_calc = {8'b0, buffer_read[15:8]} * {8'b0, data_read[15:8]};
               data_write[15:8] <= apply_clamp(temp_calc, clamp);
            end

            proc_memory_addr_counter <= proc_memory_addr_counter+1;

            if(proc_counter_read > 2) begin // > 2 and not 0 because we are shifted by one
               proc_counter_read <= proc_counter_read - 2;
            end else begin
               state_processing <= STATE_IDLE;
            end
         end
      end
   end
   STATE_PROC_CONVOLUTION: begin : conv
      reg [15:0] temp_calc; //used for calculations

      if(convolution_add_to_result == 1 && convolution_reading_data != 2'b10) begin
         if(convolution_reading_data == 2'b00) begin
            rd_en <= 1;
            addr <= proc_conv_memory_addr_write;
            convolution_reading_data <= 2'b01;
         end else if(convolution_reading_data == 2'b01 && data_read_valid == 1) begin
            convolution_data_to_add <= data_read;
            convolution_reading_data <= 2'b10;
         end
      end else if(proc_conv_memory_addr_read[0] == 1'b0) begin //read the data
         rd_en <= 1;
         addr <= proc_conv_memory_addr_read;
         proc_conv_memory_addr_read <= proc_conv_memory_addr_read + 1;


         if(convolution_add_to_result == 0) begin
            convolution_data_to_add <= 16'b0;
         end
      end else if (data_read_valid == 1) begin

         //keep the current read in a buffer before putting it in the convolution buffer because we read data 2 by 2 and we need 3x3 matrices
         convolution_previous_read[0] <= data_read;
         convolution_previous_read_counter_x[0] <= counter_convolution_x;
         convolution_previous_read_counter_y[0] <= counter_convolution_y;

         convolution_previous_read[1] <= convolution_previous_read[0];
         convolution_previous_read_counter_x[1] <= convolution_previous_read_counter_x[0];
         convolution_previous_read_counter_y[1] <= convolution_previous_read_counter_y[0];

         convolution_buffer[convolution_previous_read_counter_x[1][7:0]][convolution_previous_read_counter_y[1][0]] <= convolution_previous_read[1][7:0];
         convolution_buffer[convolution_previous_read_counter_x[1][7:0]+1][convolution_previous_read_counter_y[1][0]] <= convolution_previous_read[1][15:8];

         //counts the reads and increment y reg when x has sweept the width
         if(counter_convolution_x+2 >= img_width) begin
            counter_convolution_x <= 0;
            counter_convolution_y <= counter_convolution_y + 1;
         end else begin
            counter_convolution_x <= counter_convolution_x + 2;
         end

         if(counter_convolution_y == 0 || (counter_convolution_y == 1 && counter_convolution_x == 0)) begin
            //first/last lines and borders are 0
            wr_en <= 0;
         end else begin
            wr_en <= 1;
            proc_conv_memory_addr_write <= proc_conv_memory_addr_write + 2; //reads 2x8bits

            //offset between read and write
            //only update write counter when there is an actual write
            if(counter_convolution_x_write+2 >= img_width) begin
               counter_convolution_x_write <= 0;
               counter_convolution_y_write <= counter_convolution_y_write + 1;
            end else begin
               counter_convolution_x_write <= counter_convolution_x_write + 2;
            end

         end

         addr <= {proc_conv_memory_addr_write[31:1], 1'b0};

         //convolution for the first byte
         if(counter_convolution_y_write == 0 || counter_convolution_y_write >= img_height-1 || counter_convolution_x_write == 0) begin
            data_write[7:0] <= 0;
         end else begin
            temp_calc = 0;
            temp_calc = temp_calc + convolution_matrix[0]*{8'b0, convolution_buffer[counter_convolution_x_write[7:0]-1][ (counter_convolution_y_write[0]+1)%2]};
            temp_calc = temp_calc + convolution_matrix[1]*{8'b0, convolution_buffer[counter_convolution_x_write[7:0]][ (counter_convolution_y_write[0]+1)%2]};
            temp_calc = temp_calc + convolution_matrix[2]*{8'b0, convolution_buffer[counter_convolution_x_write[7:0]+1][ (counter_convolution_y_write[0]+1)%2]};
            temp_calc = temp_calc + convolution_matrix[3]*{8'b0, convolution_buffer[counter_convolution_x_write[7:0]-1][ (counter_convolution_y_write[0])]};
            temp_calc = temp_calc + convolution_matrix[4]*{8'b0, convolution_buffer[counter_convolution_x_write[7:0]][ (counter_convolution_y_write[0])]};
            temp_calc = temp_calc + convolution_matrix[5]*{8'b0, convolution_buffer[counter_convolution_x_write[7:0]+1][ (counter_convolution_y_write[0])]};
            //last line still not written to the buffer
            temp_calc = temp_calc + convolution_matrix[6]*{8'b0, convolution_previous_read[1][15:8]};
            temp_calc = temp_calc + convolution_matrix[7]*{8'b0, convolution_previous_read[0][7:0]};
            temp_calc = temp_calc + convolution_matrix[8]*{8'b0, convolution_previous_read[0][15:8]};

            temp_calc[7:0] = apply_clamp_fixed16(temp_calc, clamp);
            //convolution_data_to_add is either the value already existing at this address, or 0 (depends on param)
            data_write[7:0] <= apply_clamp({8'b0, convolution_data_to_add[7:0]}+{8'b0, temp_calc[7:0]}, 1);
         end

         if(counter_convolution_y_write == 0 || counter_convolution_y_write >= img_height-1 || counter_convolution_x_write >= img_width-2) begin
            data_write[15:8] <= 0;
         end else begin

            temp_calc = 0;
            temp_calc = temp_calc + convolution_matrix[0]*{8'b0, convolution_buffer[counter_convolution_x_write[7:0]][ (counter_convolution_y_write[0]+1)%2]};
            temp_calc = temp_calc + convolution_matrix[1]*{8'b0, convolution_buffer[counter_convolution_x_write[7:0]+1][ (counter_convolution_y_write[0]+1)%2]};
            temp_calc = temp_calc + convolution_matrix[2]*{8'b0, convolution_buffer[counter_convolution_x_write[7:0]+2][ (counter_convolution_y_write[0]+1)%2]};
            temp_calc = temp_calc + convolution_matrix[3]*{8'b0, convolution_buffer[counter_convolution_x_write[7:0]][ (counter_convolution_y_write[0])]};
            temp_calc = temp_calc + convolution_matrix[4]*{8'b0, convolution_buffer[counter_convolution_x_write[7:0]+1][ (counter_convolution_y_write[0])]};
            temp_calc = temp_calc + convolution_matrix[5]*{8'b0, convolution_buffer[counter_convolution_x_write[7:0]+2][ (counter_convolution_y_write[0])]};
            //last line not in buffer
            temp_calc = temp_calc + convolution_matrix[6]*{8'b0, convolution_previous_read[0][7:0]};
            temp_calc = temp_calc + convolution_matrix[7]*{8'b0, convolution_previous_read[0][15:8]};
            temp_calc = temp_calc + convolution_matrix[8]*{8'b0, data_read[7:0]};

            temp_calc[7:0] = apply_clamp_fixed16(temp_calc, clamp);
            data_write[15:8] <= apply_clamp({8'b0, convolution_data_to_add[15:8]}+{8'b0, temp_calc[7:0]}, 1);

         end

         //will cause the next read
         proc_conv_memory_addr_read <= proc_conv_memory_addr_read + 1;

         convolution_reading_data <= 2'b00;

         //end condition
         if(counter_convolution_y >= img_height+1 && counter_convolution_x+2 >= img_width)begin
            state_processing <= STATE_IDLE;
         end
      end
   end
   default: begin
   end
   endcase
end
endmodule
