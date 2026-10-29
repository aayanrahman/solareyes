.PHONY: test firmware

# ESP32C3 Dev Module with USB CDC On Boot enabled, the same settings the
# README lists for the Arduino IDE.
FQBN = esp32:esp32:esp32c3:CDCOnBoot=cdc

test:
	$(MAKE) -C test

firmware:
	arduino-cli compile --warnings all --fqbn $(FQBN) firmware/SolarEyes
