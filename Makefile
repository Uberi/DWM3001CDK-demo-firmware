build: development-environment
	docker run -v "$$(pwd)":/project uberi/qorvo-nrf52833-board /usr/local/segger_embedded_studio_V5.42a/bin/emBuild -config "Common" /project/DWM3001CDK-DW3_QM33_SDK_CLI-FreeRTOS.emProject

clean: development-environment
	docker run -v "$$(pwd)":/project uberi/qorvo-nrf52833-board /usr/local/segger_embedded_studio_V5.42a/bin/emBuild -config "Common" -clean /project/DWM3001CDK-DW3_QM33_SDK_CLI-FreeRTOS.emProject

# TODO: this uses --privileged and exposes all USB devices because SEGGER's libraries require it for some reason, it's not very good for security but it's the only way for now: https://wiki.segger.com/J-Link_Docker_Container
flash: development-environment
	docker run --privileged  -v /dev/bus/usb/:/dev/bus/usb -v "$$(pwd)/Output":/project/Output:ro uberi/qorvo-nrf52833-board nrfjprog --force -f nrf52 --program /project/Output/Common/Exe/DWM3001CDK-DW3_QM33_SDK_CLI-FreeRTOS.hex --sectorerase --verify

# TODO: this uses --privileged and exposes all USB devices because SEGGER's libraries require it for some reason, it's not very good for security but it's the only way for now: https://wiki.segger.com/J-Link_Docker_Container
stream-debug-logs:
	echo "Run this command to view debug logs: tail -f Output/debug-log.txt"
	docker run --privileged -it -v /dev/bus/usb:/dev/bus/usb -v "$$(pwd)/Output":/project/Output uberi/qorvo-nrf52833-board /usr/local/JLink_Linux_V792n_x86_64/JLinkRTTLogger -Device NRF52833_XXAA -if SWD -Speed 4000 -RTTChannel 0 /project/Output/debug-log.txt

serial-terminal:
	DEVICE_FILE=$$(ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null | while read -r dev; do if udevadm info -a -n $$dev | grep -q 'ATTRS{idVendor}=="1915"' && udevadm info -a -n $$dev | grep -q 'ATTRS{idProduct}=="520f"'; then echo "$$dev"; break; fi; done); \
	if [ -z "$$DEVICE_FILE" ]; then echo "Device not found"; exit 1; fi; \
	minicom --device $$DEVICE_FILE

development-shell: development-environment
	docker run -it -v "$$(pwd)":/project uberi/qorvo-nrf52833-board

development-environment:
	docker build -t uberi/qorvo-nrf52833-board - < Dockerfile  # build without sending any build context

save-development-environment:
	docker image save -o uberi_qorvo-nrf52833-board.tar uberi/qorvo-nrf52833-board

load-development-environment:
	docker image load -i uberi_qorvo-nrf52833-board.tar
