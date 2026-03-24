MTB_TYPE=PROJECT

# Target board/hardware (BSP).
# To change the target, it is recommended to use the Library manager
# ('make library-manager' from command line), which will also update
# Eclipse IDE launch configurations.
TARGET=APP_KIT_PSE84_AI

# Name of toolchain to use. Options include:
#
# GCC_ARM 	-- GCC is available as part of ModusToolbox Setup program
# ARM     	-- ARM Compiler (must be installed separately)
# IAR     	-- IAR Compiler (must be installed separately)
# LLVM_ARM	-- LLVM Embedded Toolchain (must be installed separately)
#
# See also: CY_COMPILER_PATH below
TOOLCHAIN=GCC_ARM

# Default build configuration. Options include:
#
# Debug -- build with minimal optimizations, focus on debugging.
# Release -- build with full optimizations
# Custom -- build with custom configuration, set the optimization flag in CFLAGS
#
# If CONFIG is manually edited, ensure to update or regenerate
# launch configurations for your IDE.
CONFIG=Debug

# Config file for postbuild sign and merge operations.
# NOTE: Check the JSON file for the command parameters
COMBINE_SIGN_JSON?=configs/boot_with_extended_boot.json

# Relative path to the shared repo location.
#
# All .mtb files have the format, <URI>#<COMMIT>#<LOCATION>. If the <LOCATION> field
# begins with $$ASSET_REPO$$, then the repo is deposited in the path specified by
# the CY_GETLIBS_SHARED_PATH variable. The default location is one directory level
# above the current app directory.
# This is used with CY_GETLIBS_SHARED_NAME variable, which specifies the directory name.
CY_GETLIBS_SHARED_PATH=../..

# Directory name of the shared repo location.
#
CY_GETLIBS_SHARED_NAME=mtb_shared


include ../common_app.mk
