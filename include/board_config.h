#pragma once

#if defined(POPUP_CONTROLLER_BOARD_REV_E_ESP32_S3)
#include "boards/pop_up_controller_v10_rev_e_esp32_s3.h"
#elif defined(POPUP_CONTROLLER_BOARD_REV_E)
#include "boards/pop_up_controller_v10_rev_e.h"
#elif defined(POPUP_CONTROLLER_BOARD_REV_C)
#include "boards/pop_up_controller_v10_rev_c.h"
#else
#include "boards/pop_up_controller_v10_rev_c.h"
#endif
