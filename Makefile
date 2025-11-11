build:
	@echo "Compiling..."
	@arduino-cli compile --clean --fqbn m5stack:esp32:m5stack_cardputer .

upload:
	@echo "Uploading..."
	@arduino-cli upload -p /dev/cu.usbmodem2101 --verbose --fqbn m5stack:esp32:m5stack_cardputer .

check_ports:
	ls -lAhog /dev/tty.*



