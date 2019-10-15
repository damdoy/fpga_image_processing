filename = top
pcf_file = io.pcf

build:
	yosys -p "synth_ice40 -blif $(filename).blif" $(filename).v
	arachne-pnr -d 5k -P sg48 -p $(pcf_file) $(filename).blif -o $(filename).asc
	# yosys -p "synth_ice40 -blif $(filename).blif -json $(filename).json" $(filename).v
	# nextpnr-ice40 --up5k --json $(filename).json --pcf $(pcf_file) --asc $(filename).asc #doesn't seem to work with the HW SPI module
	icepack $(filename).asc $(filename).bin

prog:
	iceprog -S $(filename).bin

prog_flash:
	iceprog $(filename).bin

clean:
	rm -rf $(filename).blif $(filename).asc $(filename).bin
