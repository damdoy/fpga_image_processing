/*  GIMP header image file format (RGB)*/

static unsigned int image_width = 8;
static unsigned int image_height = 8;

/*  Call this macro repeatedly.  After each use, the pixel data can be extracted  */

#define HEADER_PIXEL(data,pixel) {\
pixel[0] = (((data[0] - 33) << 2) | ((data[1] - 33) >> 4)); \
pixel[1] = ((((data[1] - 33) & 0xF) << 4) | ((data[2] - 33) >> 2)); \
pixel[2] = ((((data[2] - 33) & 0x3) << 6) | ((data[3] - 33))); \
data += 4; \
}
static const char *header_data =
	"U.$1M\\/TS=H*Q]0$LK[OL;WN>(2UG:G:J;7FO<GZ[?HJM<'RKKKKIK+CFJ;7HZ_@"
	"Q]0$Y_0DW.D9R-4%Q-$!KKKKDI[/?XN\\T]`0V>86V^@8U>(2O,CYN,3UA9'\"AI+#"
	"KKKKK+CIFZ?8GZO<GZO<DI[/@HZ_A)#!L[_PE:'2A)#!B97&A)#!AI+#CYO,;7FJ"
	"S-D)<7VN?(BYB97&>X>X?8FZ8&R=<7VN:76F;'BI6V>83%B)86V>97&B5F*3;7FJ"
	"";
