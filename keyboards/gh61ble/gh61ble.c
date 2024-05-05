#include "gh61ble.h"
#include "gpio.h"
#include "print.h"
#include "ws2812.h"
#include "debug.h"
#include "print.h"
#include "rgb_matrix.h"


/**
  * @brief  Resets the RCC clock configuration to the default reset state.
  * @param  None
  * @retval None
  */
void RCC_DeInit(void)
{
  /* Set HSION bit */
  RCC->CR |= (uint32_t)0x00000001;

  /* Reset SW, HPRE, PPRE1, PPRE2, ADCPRE and MCO bits */
  RCC->CFGR &= (uint32_t)0xF0FF0000;

  /* Reset HSEON, CSSON and PLLON bits */
  RCC->CR &= (uint32_t)0xFEF6FFFF;

  /* Reset HSEBYP bit */
  RCC->CR &= (uint32_t)0xFFFBFFFF;

  /* Reset PLLSRC, PLLXTPRE, PLLMUL and USBPRE/OTGFSPRE bits */
  RCC->CFGR &= (uint32_t)0xFF80FFFF;

  /* Disable all interrupts and clear pending bits  */
  RCC->CIR = 0x009F0000;

}

// 并不能 进 dfu 模式
void Stm32_Rest2(void) {
    __set_FAULTMASK(1);
    NVIC_SystemReset();
};


void JumpToBootloader(void)
{
	uint32_t i=0;
	void (*SysMemBootJump)(void);        /* 声明一个函数指针 */
	__IO uint32_t BootAddr = 0x1FFF0000; /* STM32F4的系统BootLoader地址 */

	/* 关闭全局中断 */
	DISABLE_INT();

	/* 关闭滴答定时器，复位到默认值 */
	SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL = 0;

	/* 设置所有时钟到默认状态，使用HSI时钟 */
	RCC_DeInit();

	/* 关闭所有中断，清除所有中断挂起标志 */
	for (i = 0; i < 8; i++)
	{
		NVIC->ICER[i]=0xFFFFFFFF;
		NVIC->ICPR[i]=0xFFFFFFFF;
	}

	/* 使能全局中断 */
	ENABLE_INT();

	/* 设置重映射到系统Flash */
	__HAL_SYSCFG_REMAPMEMORY_SYSTEMFLASH();

	/* 跳转到系统BootLoader，首地址是MSP，地址+4是复位中断服务程序地址 */
	SysMemBootJump = (void (*)(void)) (*((uint32_t *) (BootAddr + 4)));

	/* 设置朱堆栈指针 */
	__set_MSP(*(uint32_t *)BootAddr);

	/* 在RTOS工程，这条语句很重要，设置为特权级模式，使用MSP指针 */
	__set_CONTROL(0);

	/* 跳转到系统BootLoader */
	SysMemBootJump();

	/* 跳转成功的话，不会执行到这里，用户可以在这里添加代码 */
	while (1)
	{

	}
}

//void bootloader_jump(void) {
//    JumpToBootloader();
//};


void keyboard_pre_init_user(void) {
    print("gb61ble init");


}

void keyboard_post_init_user(void) {
    print("set pc14 to high");
    // enable dc pin  pc14
    palSetPad(GPIOC, 14);
    //turn off led near usb
    palClearPad(GPIOB, 5);
    // Customise these values to desired behaviour
    //debug_keyboard=true;
    //debug_mouse=true;
}

__attribute__((weak)) bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case GK_DEBUG:
            if (record->event.pressed) {
                debug_matrix=debug_enable;
            }

            printf("debug mode %b\n", debug_enable);
            printf("debug matrix %b\n", debug_matrix);

            // Do not let QMK process the keycode further
            return false;
            break;
        default:
            return true; // Process all other keycodes normally
    }
    return true;
}

#ifdef RGB_MATRIX_ENABLE
led_config_t g_led_config = {
    {
        //key to led index
        {
            60, //  0   Esc
            59, //  1   1
            58, //  2   2
            57, //  3   3
            56, //  4   4
            55, //  5   5
            54, //  6   6
            53, //  7   7
            52, //  8   8
            51, //  9   9
            50, //  10  0
            49, //  11  -
            48  //  12  =
                // 50  //  13  Backspace
        },
        {
            46, //  14  TAB
            45, //  15  Q
            44, //  16  W
            43, //  17  E
            42, //  18  R
            41, //  19  T
            40, //  20  Y
            39, //  21  U
            38, //  22  I
            37, //  23  O
            36, //  24  P
            35, //  25  [
            34 //  26  ]

                // 36, //  27  |
        },
        {
            32, //  14  CPS
            31, //  15  A
            30, //  16  S
            29, //  17  D
            28, //  18  F
            27, //  19  G
            26, //  20  H
            25, //  21  J
            24, //  22  K
            23, //  23  L
            22, //  24  ;
            21, //  25  '
            20 //  26  ENT

        },
        {
            19, //   0  SHT
            18, //   1  Z
            17, //   2  X
            16, //   3  C
            15, //   4  V
            14, //   5  B
            13, //   6  N
            12, //   7  M
            11, //   8  <
            10, //   9  >
            9, //  10  /
            8, //  12  SHT
            NO_LED
        },
        {
            7, //   0  CTR
            6, //   1  WIN
            5, //   2  ALT
            4, //   3  SPA
            3, //   4  ALT
            2, //   5  FN
            1, //   6  GUI
            0, //   7  CTR
            NO_LED,
            47, //   9  Backspace
            33, //  10  |
            NO_LED,
            NO_LED
        }
    },
    {
        //row4
        {224, 64}, //  21  CTR
        {210, 64}, //  20  GUI
        {172, 64}, //  19  FN
        {180, 64}, //  18  ALT
        {102, 64}, //  17  SPE
        {34,  64}, //  16  ALT
        {17,  64}, //  15  WIN
        { 0,  64}, //  14  CTR
                   //
        //row3
        {220, 48}, //  25  RSHT
        {203, 48}, //  24  /
        {183, 48}, //  23  >
        {163, 48}, //  22  <
        {143, 48}, //  21  M
        {122, 48}, //  20  N
        {102, 48}, //  19  B
        {81,  48}, //  18  V
        {61,  48}, //  17  C
        {41,  48}, //  16  X
        {20,  48}, //  15  Z
        { 0,  48}, //  14  LSHT
        //row2
        {224, 32}, //  27  ENT
        {207, 32}, //  25  '
        {172, 32}, //  24  ;
        {155, 32}, //  23  L
        {138, 32}, //  22  K
        {121, 32}, //  21  J
        {103, 32}, //  20  H
        {86,  32}, //  19  G
        {69,  32}, //  18  F
        {52,  32}, //  17  D
        {34,  32}, //  16  S
        {17,  32}, //  15  A
        { 0,  32}, //  14  CPS
        //row 1
        {224, 16}, //  27  |
        {207, 16}, //  26  ]
        {190, 16}, //  25  [
        {172, 16}, //  24  P
        {155, 16}, //  23  O
        {138, 16}, //  22  I
        {121, 16}, //  21  U
        {103, 16}, //  20  Y
        {86,  16}, //  19  T
        {69,  16}, //  18  R
        {52,  16}, //  17  E
        {34,  16}, //  16  W
        {17,  16}, //  15  Q
        { 0,  16}, //  14  TAB
        //row 0
        {224, 0 }, //  13 Backspace
        {207, 0 }, //  12 =
        {190, 0 }, //  11 -
        {172, 0 }, //  10 0
        {155, 0 }, //  9  9
        {138, 0 }, //  8  8
        {121, 0 }, //  7  7
        {103, 0 }, //  6  6
        {86,  0 }, //  5  5
        {69,  0 }, //  4  4
        {52,  0 }, //  3  3
        {34,  0 }, //  2  2
        {17,  0 }, //  1  1
        { 0,  0 }, //  0  Esc
    }
    , {
            1, 1, 1, 1, 4, 1, 1, 1,
            1, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 1,
            4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 1,
            4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    }
};


#endif
