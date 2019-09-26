//ice40 has 1024 kbit of spram ==> 128KB

// wire [31:0] ip_mem_addr;
// wire ip_mem_wr_en;
// wire ip_mem_rd_en;
// wire [15:0] ip_mem_data_write;
// wire [15:0] ip_mem_data_read;
// wire ip_mem_data_read_valid;

module ram_interface(input clk, input [31:0] addr, input wr_en, input rd_en, input [15:0] data_write, output [15:0] data_read, output data_read_valid);

   wire ram_wren [3:0];

   wire [15:0] ram_data_out [3:0];

   reg [1:0] output_mux;
   reg rd_en_buffer;


   SB_SPRAM256KA spram0
   (
      .ADDRESS(addr[14:1]), //14bits (16K*2B)
      .DATAIN(data_write),
      .MASKWREN(4'b1111),
      .WREN(ram_wren[0]),
      .CHIPSELECT(1'b1),
      .CLOCK(clk),
      .STANDBY(1'b0),
      .SLEEP(1'b0),
      .POWEROFF(1'b1),
      .DATAOUT(ram_data_out[0])
   );

   SB_SPRAM256KA spram1
   (
      .ADDRESS(addr[14:1]),
      .DATAIN(data_write),
      .MASKWREN(4'b1111),
      .WREN(ram_wren[1]),
      .CHIPSELECT(1'b1),
      .CLOCK(clk),
      .STANDBY(1'b0),
      .SLEEP(1'b0),
      .POWEROFF(1'b1),
      .DATAOUT(ram_data_out[1])
   );

   SB_SPRAM256KA spram2
   (
      .ADDRESS(addr[14:1]),
      .DATAIN(data_write),
      .MASKWREN(4'b1111),
      .WREN(ram_wren[2]),
      .CHIPSELECT(1'b1),
      .CLOCK(clk),
      .STANDBY(1'b0),
      .SLEEP(1'b0),
      .POWEROFF(1'b1),
      .DATAOUT(ram_data_out[2])
   );

   SB_SPRAM256KA spram3
   (
      .ADDRESS(addr[14:1]),
      .DATAIN(data_write),
      .MASKWREN(4'b1111),
      .WREN(ram_wren[3]),
      .CHIPSELECT(1'b1),
      .CLOCK(clk),
      .STANDBY(1'b0),
      .SLEEP(1'b0),
      .POWEROFF(1'b1),
      .DATAOUT(ram_data_out[3])
   );

   assign data_read = ram_data_out[output_mux];
   assign ram_wren[addr[16:15]] = wr_en;

   always @(posedge clk)
   begin
      output_mux <= addr[16:15];
      rd_en_buffer <= rd_en;
      data_read_valid <= (rd_en_buffer == 0 && rd_en == 1);
   end

endmodule
