#pragma once

#define STM32_BOOTLOADER_DUAL_BANK_GPIO FALSE

#define EARLY_INIT_PERFORM_BOOTLOADER_JUMP TRUE

#ifdef WEAR_LEVELING_EMBEDDED_FLASH
#    ifndef WEAR_LEVELING_EFL_FIRST_SECTOR
#        ifdef BOOTLOADER_TINYUF2
#            define WEAR_LEVELING_EFL_FIRST_SECTOR 3
#        else
#            define WEAR_LEVELING_EFL_FIRST_SECTOR 1
#        endif
#    endif
#endif


