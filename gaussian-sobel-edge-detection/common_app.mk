# Locate ModusToolbox helper tools folders in default installation
# locations for Windows, Linux, and macOS.
CY_WIN_HOME=$(subst \,/,$(USERPROFILE))
CY_TOOLS_PATHS ?= $(wildcard \
    $(CY_WIN_HOME)/ModusToolbox/tools_* \
    /opt/Tools/ModusToolbox/tools_* \
    /Applications/ModusToolbox/tools_*)

# If you install ModusToolbox software in a custom location, add the path to its
# "tools_X.Y" folder (where X and Y are the version number of the tools
# folder).
CY_TOOLS_PATHS+=

# Default to the newest installed tools folder, or the users override (if it's
# found).
CY_TOOLS_DIR=$(lastword $(sort $(wildcard $(CY_TOOLS_PATHS))))

# Absolute path to the compiler's "bin" directory. The variable name depends on 
# the toolchain used for the build. Refer to the ModusToolbox user guide to get 
# the correct variable name for the toolchain used in your build.
#
# The default path depends on the selected TOOLCHAIN and is set in the 
# Make recipe.
CY_COMPILER_GCC_ARM_DIR?=

ifeq ($(CY_TOOLS_DIR),)
$(error Unable to find any of the available CY_TOOLS_PATHS -- $(CY_TOOLS_PATHS))
endif
