Time Of Flight Demo
===================

A demonstration of running custom code and performing realtime range measurements between two Qorvo DWM3001CDK development kits. Features:

* Reproducible: built-in Docker development environment automates away most of the fragile and finicky parts of setting up the Qorvo SDK, SEGGER Embedded Studio, nRF52 SDK, and nRF command line tools.
* Minimal: based on the official DWM3001CDK CLI project, but significantly simpler in terms of code size and organization.
* Runs directly on the DWM3001CDK's onboard nRF52833: unlike other examples, including all of the official Qorvo SDKs, which target an external microcontroller that then interacts with the DWM3001CDK.

Quickstart:

```sh
make build

# MANUAL ACTION: connect the lower USB port of the DWM3001CDK (labelled J9) to this computer using a USB cable

make flash

# MANUAL ACTION: disconnect the lower USB port of the DWM3001CDK, and connect the upper USB port instead (labelled J20)

make serial-terminal

# you should now see a minicom instance connected to the DWM3001CDK, try entering the "help" command to see available options
```
