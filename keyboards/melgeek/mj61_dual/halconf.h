// Copyright 2026 soloy
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#ifdef BLUETOOTH_ENABLE
#    define HAL_USE_SERIAL TRUE // UART1 to the nRF52, see mj61_bt.c
#    define SERIAL_BUFFERS_SIZE 64 // four 16-byte frames
#endif

#include_next <halconf.h>
