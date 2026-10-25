# Bluetooth through the nRF52 (mj61_bt.c), only in builds that enable it (keymaps/via).
ifeq ($(strip $(BLUETOOTH_ENABLE)), yes)
    SRC += mj61_bt.c
endif
