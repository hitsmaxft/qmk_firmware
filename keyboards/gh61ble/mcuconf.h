#pragma once

//load default
#include_next <mcuconf.h>

#define BOARD_OTG_NOVBUSSENS

#define STM32_HSE_ENABLED                   TRUE

#undef STM32_USB_USE_OTG1
#define STM32_USB_USE_OTG1          FALSE
#define GPIOA_OTG_FS_DM             11U
#define GPIOA_OTG_FS_DP             12U
