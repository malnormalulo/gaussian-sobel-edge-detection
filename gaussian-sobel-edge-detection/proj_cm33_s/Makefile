include ../common.mk

# Name of application (used to derive name of final linked file).
#
# If APPNAME is edited, ensure to update or regenerate launch
# configurations for your IDE.
APPNAME=proj_cm33_s

# If set to "true" or "1", display full command-lines when building.
VERBOSE=

# Enable optional code that is ordinarily disabled by default.
#
# Available components depend on the specific targeted hardware and firmware
# in use. In general, if you have
#
#    COMPONENTS=foo bar
#
# ... then code in directories named COMPONENT_foo and COMPONENT_bar will be
# added to the build
#
COMPONENTS+=SECURE_DEVICE

CORE=CM33
CORE_NAME=CM33_0

# Set this application to be secure
VCORE_ATTRS=SECURE

#Configure this application to be loaded into flash/ram.
APPTYPE=flash

include $(CY_TOOLS_DIR)/make/start.mk
