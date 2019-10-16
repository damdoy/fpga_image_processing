#include <stdio.h>
#include <string.h>
#include "spi_lib.h"

static struct ftdi_context ftdic;
static int ftdic_open = 0; //false
static int verbose = 0; //false
static int ftdic_latency_set = 0; //false
static unsigned char ftdi_latency;

static void send_byte(uint8_t data)
{
  int rc = ftdi_write_data(&ftdic, &data, 1);
  if (rc != 1) {
     fprintf(stderr, "Write error (single byte, rc=%d, expected %d) data: 0x%x.\n", rc, 1, data);
     exit(2);
  }
}

static uint8_t recv_byte()
{
  uint8_t data;
  while (1) {
     int rc = ftdi_read_data(&ftdic, &data, 1);
     if (rc < 0) {
        fprintf(stderr, "Read error.\n");
        exit(2);
     }
     if (rc == 1)
        break;
     usleep(100);
  }
  return data;
}

static uint8_t xfer_spi_bits(uint8_t data, int n)
{
  if (n < 1)
     return 0;

  /* Input and output, update data on negative edge read on positive, bits. */
  send_byte(MC_DATA_IN | MC_DATA_OUT | MC_DATA_BITS | MC_DATA_LSB );
  send_byte(n - 1);
  send_byte(data);

  return recv_byte();
}

static void xfer_spi(uint8_t *data, int n)
{
  if (n < 1)
     return;

  /* Input and output, update data on negative edge read on positive. */
  send_byte(MC_DATA_IN | MC_DATA_OUT | MC_DATA_LSB | MC_DATA_OCN);
  send_byte(n - 1);
  send_byte((n - 1) >> 8);

  int rc = ftdi_write_data(&ftdic, data, n);
  if (rc != n) {
     fprintf(stderr, "Write error (chunk, rc=%d, expected %d).\n", rc, n);
     exit(2);
  }

  for (int i = 0; i < n; i++)
     data[i] = recv_byte();
}

static void send_spi(uint8_t *data, int n)
{
  if (n < 1)
     return;

  //sends data on the negative edges
  send_byte(MC_DATA_OUT | MC_DATA_LSB | MC_DATA_OCN);
  send_byte(n - 1);
  send_byte((n - 1) >> 8);

  int rc = ftdi_write_data(&ftdic, data, n);
  if (rc != n) {
     fprintf(stderr, "Write error (chunk, rc=%d, expected %d).\n", rc, n);
     exit(2);
  }
}

static void read_spi(uint8_t *data, int n)
{
  if (n < 1)
     return;

  //receive data on positive edge
  send_byte(MC_DATA_IN | MC_DATA_LSB /*| MC_DATA_OCN*/);
  send_byte(n - 1);
  send_byte((n - 1) >> 8);

  // int rc = ftdi_write_data(&ftdic, data, n);
  // if (rc != n) {
  //    fprintf(stderr, "Write error (chunk, rc=%d, expected %d).\n", rc, n);
  //    error(2);
  // }

  for (int i = 0; i < n; i++)
     data[i] = recv_byte();
}


static void set_gpio(int slavesel_b, int creset_b)
{
  uint8_t gpio = 0;

  if (slavesel_b) {
     // ADBUS4 (GPIOL0)
     gpio |= 0x10;
  }

  if (creset_b) {
     // ADBUS7 (GPIOL3)
     gpio |= 0x80;
  }

  send_byte(MC_SETB_LOW);
  send_byte(gpio); /* Value */
  send_byte(0x93); /* Direction */
}

// the FPGA reset is released so also FLASH chip select should be deasserted
static void flash_release_reset()
{
  set_gpio(1, 1);
}

// SRAM reset is the same as flash_chip_select()
// For ease of code reading we use this function instead
static void sram_reset()
{
  // Asserting chip select and reset lines
  set_gpio(0, 0);
}

// SRAM chip select assert
// When accessing FPGA SRAM the reset should be released
static void sram_chip_select()
{
  set_gpio(0, 1);
}


int spi_init()
{
   // ftdi initialization taken from iceprog https://github.com/cliffordwolf/icestorm/blob/master/iceprog/iceprog.c
   enum ftdi_interface ifnum = INTERFACE_A;
   int status = 0;

   printf("init..\n");

   status = ftdi_init(&ftdic);
	if( status != 0)
   {
      printf("couldn't initalize ftdi\n");
      return 1;
   }

	status = ftdi_set_interface(&ftdic, ifnum);
   if(status != 0)
   {
      printf("couldn't initalize ftdi interface\n");
      return 1;
   }

   if (ftdi_usb_open(&ftdic, 0x0403, 0x6010) && ftdi_usb_open(&ftdic, 0x0403, 0x6014)) {
		printf("Can't find iCE FTDI USB device (vendor_id 0x0403, device_id 0x6010 or 0x6014).\n");
		return 1;
	}

   if (ftdi_usb_reset(&ftdic)) {
		fprintf(stderr, "Failed to reset iCE FTDI USB device.\n");
		return 1;
	}

	if (ftdi_usb_purge_buffers(&ftdic)) {
		fprintf(stderr, "Failed to purge buffers on iCE FTDI USB device.\n");
		return 1;
	}

	if (ftdi_get_latency_timer(&ftdic, &ftdi_latency) < 0) {
		fprintf(stderr, "Failed to get latency timer (%s).\n", ftdi_get_error_string(&ftdic));
		return 1;
	}

	/* 1 is the fastest polling, it means 1 kHz polling */
	if (ftdi_set_latency_timer(&ftdic, 1) < 0) {
		fprintf(stderr, "Failed to set latency timer (%s).\n", ftdi_get_error_string(&ftdic));
		return 1;
	}

	// ftdic_latency_set = 1;

	/* Enter MPSSE (Multi-Protocol Synchronous Serial Engine) mode. Set all pins to output. */
	if (ftdi_set_bitmode(&ftdic, 0xff, BITMODE_MPSSE) < 0) {
		fprintf(stderr, "Failed to set BITMODE_MPSSE on iCE FTDI USB device.\n");
		exit(2);
	}

   //enable clock divide by 5 ==> 6MHz
   send_byte(MC_TCK_D5);

   //divides by value+1
	send_byte(MC_SET_CLK_DIV);
	send_byte(0);
	send_byte(0x01);
   //so, 6/2 MHz ==> 3MHz

	usleep(100);

	sram_chip_select();
	usleep(2000);

   return 0;
}

//send without caring about result
int spi_command_send(uint8_t cmd, uint8_t param[3]){
   uint8_t nop_param[] = {0,0}; //wont be returned

   return spi_command_send_recv(cmd, param, nop_param);
}

//send one byte without caring about result
int spi_command_send(uint8_t cmd, uint8_t param){
   uint8_t nop_param[] = {0,0}; //wont be returned
   uint8_t params[] = {param, 0,0};

   return spi_command_send_recv(cmd, params, nop_param);
}

//sends 16bits
int spi_command_send_16B(uint8_t cmd, uint8_t data[16]){
   uint8_t nop_param[15]; //wont be returned

   return spi_command_send_recv_16B(cmd, data, nop_param);
}

int spi_command_send_recv(uint8_t cmd, uint8_t send_param[3], uint8_t recv_data[2])
{
   uint8_t to_send[] = {cmd, send_param[0], send_param[1], send_param[2]};
   uint retries = 0;
   uint max_retries = 10;

   do{
      to_send[0] = cmd;
      memcpy(to_send+1, send_param, 7);
      xfer_spi(to_send, 4);
      retries++;
   } while(retries < max_retries && (to_send[2] & STATUS_FPGA_RECV_MASK) == 0); //check that fpga really received the datas

   //copy data received to output
   //first 2 bytes are garbage, the third one is the status
   memcpy(recv_data, to_send+2, 2);

   return retries >= max_retries;
}

//TODO merge with last function
int spi_command_send_recv_16B(uint8_t cmd, uint8_t send_param[16], uint8_t recv_data[15])
{
   uint8_t to_send[17];
   uint retries = 0;
   uint max_retries = 10;

   do{
      to_send[0] = cmd;
      memcpy(to_send+1, send_param, 16);
      xfer_spi(to_send, 17);
      retries++;
   } while(retries < max_retries && (to_send[2] & STATUS_FPGA_RECV_MASK) == 0); //check that fpga really received the datas

   //copy data received to output
   //first 2 bytes are garbage, the third one is the status
   memcpy(recv_data, to_send+2, 15);

   return retries >= max_retries;
}
