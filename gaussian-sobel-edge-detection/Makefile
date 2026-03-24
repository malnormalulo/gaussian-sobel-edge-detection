################################################################################
# \file Makefile
# \version 1.0
#
# \brief
# Top-level application make file.
#
################################################################################
# \copyright
# Copyright 2023-2025, Cypress Semiconductor Corporation (an Infineon company)
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
################################################################################


################################################################################
# Basic Configuration
################################################################################

# Type of ModusToolbox Makefile Options include:
#
# COMBINED    -- Top Level Makefile usually for single standalone application
# APPLICATION -- Top Level Makefile usually for multi project application
# PROJECT     -- Project Makefile under Application
#
MTB_TYPE=APPLICATION

MTB_PROJECTS=proj_cm33_s proj_cm33_ns proj_cm55

################################################################################
# Paths
################################################################################

include common_app.mk


include $(CY_TOOLS_PATHS)/make/application.mk

dasm: build
	/opt/Tools/mtb-gcc-arm-eabi/14.2.1/gcc/bin/arm-none-eabi-objdump -S --source-comment="@" -m armv8.1-m.main build/project_hex/proj_cm55.elf > build/project_hex/proj_cm55.asm
