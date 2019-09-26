echo "TODO"

g++ -DICE40 ice40/software/spi_lib/spi_lib.c ice40/software/image_processing_ice40.cpp software/main.cpp -o soft_ice40 -lftdi
# g++ -DICE40 ice40/software/spi_lib/spi_lib.c ice40/software/image_processing_ice40.cpp  -o soft_ice40
