#
# This file has been automatically generated by the WICED 20719-B1 Designer.
#

APP_SRC = key_LED.c
APP_SRC += key_LED_db.c
APP_SRC += wiced_bt_cfg.c
APP_SRC += custom_pin_config.c

C_FLAGS += -DWICED_BT_TRACE_ENABLE
C_FLAGS += -DSMUX_CHIP=$(CHIP)

# If defined, HCI traces are sent over transport/WICED HCI interface
C_FLAGS += -DHCI_TRACE_OVER_TRANSPORT

########################################################################
################ DO NOT MODIFY FILE BELOW THIS LINE ####################
########################################################################

