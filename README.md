# Gaussian Blur + Sobel Edge Detection on ARM Cortex-M55

Image processing pipeline accelerated with ARM Helium (MVE) vector intrinsics on an Infineon PSOC Edge E84 MCU (Cortex-M55).

## Pipeline

RGB image → **monochrome** → **Gaussian blur** → **Sobel edge detection** → binary edge map

## Implementations

### Baseline (`baseline SW implementation/`)

Plain C, runs on the host. Reference for correctness and algorithm design.

- **Monochrome**: weighted float sum — `0.3R + 0.59G + 0.11B`
- **Gaussian blur**: 2D float kernel convolution, boundary pixels skip out-of-bounds neighbors
- **Sobel**: 2D `G_x`/`G_y` kernels, `sqrt(Gx²+Gy²)` magnitude, fixed threshold of 25

### CM55 — naive (`proj_cm55/core_naive.h`)

Direct port of the baseline to the MCU. Same 2D float kernels, no vectorization.

### CM55 — optimized (`proj_cm55/core.h`)

Helium MVE-vectorized implementation with several algorithmic improvements:

| Stage | Optimization |
|---|---|
| Monochrome | Gather-load 8 pixels/cycle; integer coefficients (77/151/28), right-shift instead of float multiply |
| Gaussian blur | **Separable 1×K kernel** (horizontal then vertical pass via `gauss_buffer`); uint8 fixed-point weights scaled to ×256; inner loop processes 8 pixels/cycle with `vmlaq_n_u16` |
| Sobel | **Separable kernels** — horizontal pass (smoothing or derivative) writes `sobel_buffer` (int16), vertical pass completes G_x/G_y; absolute value accumulated into a sum used to compute a **dynamic threshold** (mean gradient); final thresholding also vectorized |

Key MCU-specific details:
- Hot buffers (`gauss_buffer`, `sobel_buffer`) placed in `.cy_dtcm` (tightly coupled data memory)
- All inner-loop functions placed in `.cy_itcm` (tightly coupled instruction memory) and marked `NO_INLINE` for accurate profiling
- Image fixed at 128×128 (`SIZE = HEIGHT * WIDTH`) to fit in TCM

## Accuracy vs reference

`main.c` compares each stage against pre-computed reference outputs (`out_monochrome.h`, `out_gaussian_blur.h`, `out_sobel_edge.h`) and prints `max_err`, `mean_err`, `% exact`, and `% within ±4`.

## Performance metrics reported

`cycles`, `instructions`, `MAC/cycle`, `IPC`, `cycles/pixel` — measured via the PMU-based `perf_counter`.

## Branch: `camera-module-setup`

Extends the pipeline to run live on an **OV7675 camera module**, replacing the static test image with a continuous capture loop.

### What changes

**Input format** — the camera delivers frames in RGB565. Two new conversion functions handle the boundaries of the pipeline:
- `convert_rgb565_to_mono_rgb888` — converts a camera frame to grayscale (vectorized with Helium)
- `mono_rgb888_to_rgb565` — converts the edge-detection output back to RGB565 for display/transmission

**Main loop** — instead of a one-shot benchmark, the MCU runs a frame loop:
```
capture frame (RGB565) → RGB565→mono → gaussian blur → sobel → mono→RGB565 → output
```

**Resolution** — commit `5f83011` switches from QVGA (320×240) to **VGA (640×480)** by enabling `OV7675_RES_VGA` in the Makefile. The Gaussian kernel is widened accordingly (SIGMA 1→2, kernel 3→5).

**Memory layout** — at 640×480 the image buffers no longer fit in DTCM, so:
- Intermediate pipeline buffers (`out_monochrome`, `out_gaussian_blur`, `out_sobel`) are consolidated into a single `shared_buffer` in `.cy_socmem_bss`
- `sobel_buffer` moved from `.cy_dtcm` to `.cy_socmem_data`
- The loop reuses `comm_buffer` and the active camera frame buffer as ping-pong intermediates to avoid extra allocations

### Running

Two terminals are needed — one for the MCU, one for the host visualizer.

**Terminal 1 — flash the MCU**

```bash
cd gaussian-sobel-edge-detection/
make program
```

**Terminal 2 — host visualizer**

Set up the Python environment once:

```bash
python3 -m venv .venv
source .venv/bin/activate
pip install pyserial opencv-python
```

Then launch (adjust `--port` to your device):

```bash
source .venv/bin/activate
python visual.py --port /dev/ttyACM1
```

Optional argument: `--baud` (default `1000000`).

**How it works** — `visual.py` sends a `'1'` byte to trigger each frame. The MCU replies with a 4-byte sync word (`0xF81F F81F`), a 4-byte little-endian length, and the raw RGB565 frame. The script auto-detects QVGA (320×240) and VGA (640×480) from the payload size, unpacks RGB565 to BGR, and shows a live OpenCV window. Press **q** to quit.

---

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
