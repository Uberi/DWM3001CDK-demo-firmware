build: development-environment
	docker run -v "$$(pwd)/DWM3001CDK-DW3_QM33_SDK-FreeRTOS":/project:ro -v "$$(pwd)/build_output":/project/Output uberi/qorvo-nrf52833-board /usr/local/segger_embedded_studio_V7.32/bin/emBuild -config "Common" /project/DWM3001CDK-DW3_QM33_SDK_CLI-FreeRTOS.emProject

# TODO: this uses --privileged and exposes all USB devices because SEGGER's libraries require it for some reason, it's not very good for security but it's the only way for now: https://wiki.segger.com/J-Link_Docker_Container
flash: development-environment
	docker run --privileged  -v /dev/bus/usb/:/dev/bus/usb -v "$$(pwd)/build_output":/project/Output:ro uberi/qorvo-nrf52833-board nrfjprog --force -f nrf52 --program /project/Output/Common/Exe/DWM3001CDK-DW3_QM33_SDK_CLI-FreeRTOS.hex --sectorerase --verify

serial-terminal:
	DEVICE_FILE=$$(ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null | while read -r dev; do if udevadm info -a -n $$dev | grep -q 'ATTRS{idVendor}=="1915"' && udevadm info -a -n $$dev | grep -q 'ATTRS{idProduct}=="520f"'; then echo "$$dev"; break; fi; done); \
	if [ -z "$$DEVICE_FILE" ]; then echo "Device not found"; exit 1; fi; \
	minicom --device $$DEVICE_FILE

development-shell: development-environment
	docker run -it -v "$$(pwd)/DWM3001CDK-DW3_QM33_SDK-FreeRTOS":/project -v "$$(pwd)/build_output":/project/Output uberi/qorvo-nrf52833-board

development-environment:
	docker build -t uberi/qorvo-nrf52833-board .
