

#include "STM32_SysControl.h"
#include "stm32f0xx.h"

STM32_Syscontrol_Config_t SysConfig;

/*************************************************************************
 * Function Name: SYS_Init
 * Parameters:  unsigned long Fosc
 *	        unsigned long Fcpu
 * 		STM32_SysControl_AHBPrescaler_t AHBPrescaler
 *		STM32_SysControl_APBPrescaler_t APBPrescaler
 *		bool PLL_Enable
 *		STM32_SysControl_PLLConfig_t *PLLConfig
 *              uint32_t PortDir
 *              uint32_t PortOut
 *
 * Return: int
 *             	0: success
 *	 non-zero: error number
 *
 * Description: Initialize the whole system, setting PLL, AHB, APB, GPIO
 *
 *************************************************************************/
int SYS_Init(unsigned long Fosc, unsigned long Fcpu,
             STM32_SysControl_AHBPrescaler_t AHBPrescaler,
             STM32_SysControl_APBPrescaler_t APBPrescaler,
             bool PLL_Enable,
             STM32_SysControl_PLLConfig_t *PLLConfig,
             uint32_t PortDir,
             uint32_t PortOut)
{
    // Check validity of input parameters
    if (Fosc < FOSC_MIN || Fosc > FOSC_MAX || Fcpu < FCPU_MIN || Fcpu > FCPU_MAX || Fcpu > Fosc)
        return 1;

    // Set global variables
    SysConfig.Fosc = Fosc;
    SysConfig.Fcpu = Fcpu;
    SysConfig.AHBPrescaler = AHBPrescaler;
    SysConfig.APBPrescaler = APBPrescaler;

    // Configure PLL if enabled
    if (PLL_Enable)
    {
        // Configure PLL settings
        RCC->CR |= RCC_CR_HSION; // Enable internal high-speed clock
        while (!(RCC->CR & RCC_CR_HSIRDY))
            ; // Wait until HSI is ready

        RCC->CFGR &= ~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLXTPRE | RCC_CFGR_PLLMUL); // Clear PLL settings
        RCC->CFGR |= (PLLConfig->PLLSource | PLLConfig->PLLMul);               // Set PLL source and multiplier

        RCC->CR |= RCC_CR_PLLON; // Enable PLL
        while (!(RCC->CR & RCC_CR_PLLRDY))
            ; // Wait until PLL is ready

        RCC->CFGR &= ~RCC_CFGR_HPRE; // Clear AHB prescaler
        RCC->CFGR |= AHBPrescaler;   // Set AHB prescaler

        RCC->CFGR &= ~RCC_CFGR_PPRE; // Clear APB prescaler
        RCC->CFGR |= APBPrescaler;   // Set APB prescaler
    }

    // Configure GPIO
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN; // Enable GPIOA clock
    GPIOA->MODER &= ~PortDir;          // Clear direction bits
    GPIOA->MODER |= PortOut;           // Set direction bits
    GPIOA->ODR = 0;                    // Clear output data register

    return 0;
}

/*************************************************************************
 * Function Name: SYS_GetFcpu
 * Parameters: void
 * Return: int
 *             	0: success
 *	 non-zero: error number
 * Description: Get Fcpu
 *
 *************************************************************************/
unsigned int SYS_GetFcpu(void)
{
    return SysConfig.Fcpu;
}

/*************************************************************************
 * Function Name: RCC_SetAPBPeriphClock
 * Parameters: uint32_t Periph, FunctionalState NewState
 * Return: int
 *             	0: success
 *	 non-zero: error number
 * Description: Enable or disable APB peripheral clock
 *
 *************************************************************************/
int RCC_SetAPBPeriphClock(uint32_t Periph, FunctionalState NewState)
{
    if (NewState != DISABLE)
    {
        RCC->APBENR |= Periph;
    }
    else
    {
        RCC->APBENR &= ~Periph;
    }
    return 0;
}

/*************************************************************************
 * Function Name: EXTI_Init
 * Parameters: uint32_t EXTI_Line, EXTI_Mode_TypeDef EXTI_Mode, EXTI_Trigger_TypeDef EXTI_Trigger
 * Return: int
 *             	0: success
 *	 non-zero: error number
 * Description: Configure EXTI Line
 *
 *************************************************************************/
int EXTI_Init(uint32_t EXTI_Line, EXTI_Mode_TypeDef EXTI_Mode, EXTI_Trigger_TypeDef EXTI_Trigger)
{
    if (EXTI_Line < EXTI_Line0 || EXTI_Line > EXTI_Line15)
        return 1;

    EXTI->IMR |= EXTI_Line;
    EXTI->RTSR &= ~EXTI_Line;
    EXTI->FTSR &= ~EXTI_Line;

    if (EXTI_Mode == EXTI_Mode_Interrupt)
    {
        EXTI->IMR |= EXTI_Line;
    }
    else if (EXTI_Mode == EXTI_Mode_Event)
    {
        EXTI->EMR |= EXTI_Line;
    }
    else
    {
        return 1;
    }

    if (EXTI_Trigger == EXTI_Trigger_Rising)
    {
        EXTI->RTSR |= EXTI_Line;
    }
    else if (EXTI_Trigger == EXTI_Trigger_Falling)
    {
        EXTI->FTSR |= EXTI_Line;
    }
    else if (EXTI_Trigger == EXTI_Trigger_Rising_Falling)
    {
        EXTI->RTSR |= EXTI_Line;
        EXTI->FTSR |= EXTI_Line;
    }
    else
    {
        return 1;
    }

    return 0;
}
