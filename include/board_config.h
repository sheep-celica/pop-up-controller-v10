#pragma once

#if defined(POPUP_CONTROLLER_BOARD_REV_D_ESP32_S3)
#include "boards/pop_up_controller_v10_rev_d_esp32_s3.h"
#elif defined(POPUP_CONTROLLER_BOARD_REV_D)
#include "boards/pop_up_controller_v10_rev_d.h"
#elif defined(POPUP_CONTROLLER_BOARD_REV_C)
#include "boards/pop_up_controller_v10_rev_c.h"
#else
#include "boards/pop_up_controller_v10_rev_c.h"
#endif
