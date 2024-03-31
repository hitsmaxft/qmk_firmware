#include "gh61ble.h"
#include "gpio.h"
#include "print.h"

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

void bootloader_jump(void) {
    JumpToBootloader();
};


void keyboard_pre_init_user(void) {
    print("keyboard init");
}

void keyboard_post_init_user(void) {
    print("set pc14 to high");
    // enable dc pin  pc14
    palSetPad(GPIOC, 14);
}
