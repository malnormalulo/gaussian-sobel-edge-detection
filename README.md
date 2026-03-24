# Setup

## Install Modus Toolbox and its dependencies

Tested on Ubuntu 24.04

Download the following packages:
* [ModusToolbox Tools Package](https://softwaretools.infineon.com/tools/com.ifx.tb.tool.modustoolbox?_gl=1*9final*_gcl_au*NDAxODg4MzU5LjE3NzA2NDIwNzI.*_ga*MTc5MTQyOTU2My4xNzcwNjQyMDc0*_ga_KVD0BL538B*czE3NzA2NDIwNzMkbzEkZzEkdDE3NzA2NDI2NjMkajUzJGwwJGgxMjg5Mzg1NjY1)
* [ModusToolbox Programming Tools](https://softwaretools.infineon.com/tools/com.ifx.tb.tool.modustoolboxprogtools?_gl=1*1w0end9*_gcl_au*NDAxODg4MzU5LjE3NzA2NDIwNzI.*_ga*MTc5MTQyOTU2My4xNzcwNjQyMDc0*_ga_KVD0BL538B*czE3NzA2NDIwNzMkbzEkZzEkdDE3NzA2NDI5NzkkajU1JGwwJGgxMjg5Mzg1NjY1)
* [ModusToolbox Edge Protect Security Suite](https://softwaretools.infineon.com/tools/com.ifx.tb.tool.modustoolboxedgeprotectsecuritysuite?_gl=1*7ca7sx*_gcl_au*NDAxODg4MzU5LjE3NzA2NDIwNzI.*_ga*MTc5MTQyOTU2My4xNzcwNjQyMDc0*_ga_KVD0BL538B*czE3NzA2NDIwNzMkbzEkZzEkdDE3NzA2NDQ5MjgkajQ5JGwwJGgxMjg5Mzg1NjY1)
* [Arm GNU Toolchain](https://softwaretools.infineon.com/tools/com.ifx.tb.tool.mtbgccpackage?_gl=1*svltja*_gcl_au*NDAxODg4MzU5LjE3NzA2NDIwNzI.*_ga*MTc5MTQyOTU2My4xNzcwNjQyMDc0*_ga_KVD0BL538B*czE3NzA2NDIwNzMkbzEkZzEkdDE3NzA2NDM5MjAkajEkbDAkaDEyODkzODU2NjU.)

Install them:

```bash
sudo apt install ./modustoolbox_3.7.0.18135_Linux_x64.deb
sudo apt install ./ModusToolboxProgtools_1.7.0.1727.deb
sudo apt install ./modustoolboxedgeprotectsecuritysuite_1.6.1.525_Linux_x64.deb
sudo apt install ./mtbgccpackage_14.2.1.265_Linux_x64.deb
```

logout and login or reboot

## Vscode and plugins

Install vscode if not already installed.

Install these plugins:

* C/C++ Extension Pack
* Cortex-Debug
* Serial Monitor
* Arm Assembly

via cmdline:

```bash
code --install-extension ms-vscode.cpptools-extension-pack
code --install-extension marus25.cortex-debug
code --install-extension ms-vscode.vscode-serial-monitor
code --install-extension dan-c-underwood.arm
```

# Example workflow

## Build

```
cd hello_world
```

First, we need to open the library manager and add a BSP (board support package):

```
/opt/Tools/ModusToolbox/tools_3.7/library-manager/library-manager
```

In the GUI, click on `Add BSP`. Choose the `KIT_PSE84_AI` (Under `PSOC Edge BSPs`) in the new window and click `OK`.
Under `BSPs`, click the radiobutton under the newly added BSP to activate it.
Now click on 'Update' to generate the BSP and close the library manager afterwards.

Build the application with:

```bash
make build
```

## Flash/program

Connect the MCU board and download a program to the board with:

```bash
make program
```

## Develop in vscode

First you need to generate the required IDE config files from the project with:

```bash
make vscode
```

This will generate a `hello_world.code-workspace` file among other config files. You can now open this file in vscode via `File->Open Workspace from file...`

## On-chip debugging in vscode

Once a project is opened, click on `Run and Debug` in the left panel.
Select `Launch PSOCE84 CM55 (KitProg3_MiniProg4) (proj_cm55)` and click on the play button (or hit F5). This will program the app and start the debugger.

You can also attach a debugging session to already flashed code by selecting `Attach PSOCE84 CM55 (KitProg3_MiniProg4) (proj_cm55)` instead.

## Inspecting disassembly

The following make target will generate a disassembly listing (`proj_cm55` only):

```bash
make dasm
```

Output path is `build/project_hex/proj_cm55.asm`

# Documentation

## Software

* [ModusToolbox Documentation](https://documentation.infineon.com/modustoolbox/docs/zfi1758821717322)
* [Peripheral Driver Library (PDL) API reference manual](https://infineon.github.io/mtb-dsl-pse8xxgp/html/index.html)

## Hardware

### Infineon

* [PSOC Edge E84 AI Kit datasheet](https://www.infineon.com/assets/row/public/documents/30/44/infineon-kit-pse84-ai-user-guide-usermanual-en.pdf)
* [E84 PSOC Edge MCU datasheet](https://www.infineon.com/assets/row/public/documents/30/49/infineon-psoc-edge-e8x-consumer-datasheet-datasheet-en.pdf)
* [Selecting and configuring memories for power and performance in PSOC™ Edge MCU](https://www.infineon.com/assets/row/public/documents/30/42/infineon-an239774-selecting-and-configuring-memories-power-performance-psoc-edge-applicationnotes-en.pdf)

### Cortex-M55

* [Helium Technology Reference book](https://github.com/arm-university/Arm-Helium-Technology/blob/main/HeliumTechnology_referencebook.pdf)
* [Helium Programmers Guide](https://developer.arm.com/-/media/Arm%20Developer%20Community/PDF/Helium%20Programmers%20Guide/Introduction%20to%20Heliumpdf.pdf?revision=8b4eb9ae-29ef-464f-a043-ddc2ebf9e8d1)
* [Arm Cortex-M55 Processor Software Optimization Guide](https://documentation-service.arm.com/static/654414b13f12c06bc0f7d000?token=)
