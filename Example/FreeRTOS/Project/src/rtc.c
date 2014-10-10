#include "rtc.h"
#define LSE_MAX_TRIALS_NB         6

RTC_InitTypeDef   RTC_InitStructure;
uint32_t RTC_Timeout = 0x10000;
int8_t RTC_Error = 0;
uint8_t RTC_HandlerFlag;
uint8_t __IO alarm_now = 1;
__IO uint32_t LsiFreq = 0;
__IO uint32_t CaptureNumber = 0, PeriodValue = 0;

uint32_t GetLSIFrequency(void);

/**
* @brief  Configures the RTC peripheral.
* @param  None
* @retval None
*/
int8_t RTC_Configuration(void)
{

  RTC_Error = 0;
  /* Enable the PWR clock */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);

  /* Allow access to RTC */
  PWR_BackupAccessCmd(ENABLE);

/* LSI used as RTC source clock */
/* The RTC Clock may varies due to LSI frequency dispersion. */   
  /* Enable the LSI OSC */ 
 // RCC_LSICmd(ENABLE);
	RCC_LSEConfig(RCC_LSE_ON);

  /* Wait till LSI is ready */  
  //while(RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET)
	while(RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET)
  {
  }

  /* Select the RTC Clock Source */
  //RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);
	RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
   
  /* Enable the RTC Clock */
  RCC_RTCCLKCmd(ENABLE);
  
    /* Wait for RTC APB registers synchronisation */
  RTC_WaitForSynchro();
	
	//RTC_WaitForLastTask();
  
  /* Calendar Configuration with LSI supposed at 32KHz */
  RTC_InitStructure.RTC_AsynchPrediv = 0x7F;
  RTC_InitStructure.RTC_SynchPrediv	=  0xFF; /* (32KHz / 128) - 1 = 0xFF*/
  RTC_InitStructure.RTC_HourFormat = RTC_HourFormat_24;
  RTC_Init(&RTC_InitStructure);  

  /* Get the LSI frequency:  TIM5 is used to measure the LSI frequency */
  //LsiFreq = GetLSIFrequency();
   
  /* Adjust LSI Configuration */
  RTC_InitStructure.RTC_AsynchPrediv = 0x7F;
  //RTC_InitStructure.RTC_SynchPrediv	=  (LsiFreq/128) - 1;
	RTC_InitStructure.RTC_SynchPrediv	= (32767/128) - 1;
  RTC_InitStructure.RTC_HourFormat = RTC_HourFormat_24;
  RTC_Init(&RTC_InitStructure);
  return RTC_Error;
}
/**
  * @brief  Configures TIM5 to measure the LSI oscillator frequency. 
  * @param  None
  * @retval LSI Frequency
  */
uint32_t GetLSIFrequency(void)
{
  NVIC_InitTypeDef   NVIC_InitStructure;
  TIM_ICInitTypeDef  TIM_ICInitStructure;

  /* Enable the LSI oscillator ************************************************/
  RCC_LSICmd(ENABLE);
  
  /* Wait till LSI is ready */
  while (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET)
  {}

  /* TIM5 configuration *******************************************************/ 
  /* Enable TIM5 clock */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE);
  
  /* Connect internally the TIM5_CH4 Input Capture to the LSI clock output */
  TIM_RemapConfig(TIM5, TIM5_LSI);

  /* Configure TIM5 presclaer */
  TIM_PrescalerConfig(TIM5, 0, TIM_PSCReloadMode_Immediate);
  
  /* TIM5 configuration: Input Capture mode ---------------------
     The LSI oscillator is connected to TIM5 CH4
     The Rising edge is used as active edge,
     The TIM5 CCR4 is used to compute the frequency value 
  ------------------------------------------------------------ */
  TIM_ICInitStructure.TIM_Channel = TIM_Channel_4;
  TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;
  TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
  TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV8;
  TIM_ICInitStructure.TIM_ICFilter = 0;
  TIM_ICInit(TIM5, &TIM_ICInitStructure);
  
  /* Enable TIM5 Interrupt channel */
  NVIC_InitStructure.NVIC_IRQChannel = TIM5_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  /* Enable TIM5 counter */
  TIM_Cmd(TIM5, ENABLE);

  /* Reset the flags */
  TIM5->SR = 0;
    
  /* Enable the CC4 Interrupt Request */  
  TIM_ITConfig(TIM5, TIM_IT_CC4, ENABLE);


  /* Wait until the TIM5 get 2 LSI edges (refer to TIM5_IRQHandler() in 
    stm32fxxx_it.c file) ******************************************************/
  while(CaptureNumber != 2)
  {
  }
  /* Deinitialize the TIM5 peripheral registers to their default reset values */
  TIM_DeInit(TIM5);
  
  /* Get PCLK1 prescaler */
  if ((RCC->CFGR & RCC_CFGR_PPRE1) == 0)
  { 
    /* PCLK1 prescaler equal to 1 => TIMCLK = PCLK1 */
    return (((SystemCoreClock/4) / PeriodValue) * 8);
  }
  else
  { /* PCLK1 prescaler different from 1 => TIMCLK = 2 * PCLK1 */
    return (((2 * (SystemCoreClock/4)) / PeriodValue) * 8) ;
  }
}

/**
* @brief  This function handles RTC Alarm A interrupt request.
* @param  None
* @retval None
*/
void RTC_Alarm_IRQHandler(void)
{
  /* Clear the EXTIL line 17 */
  //EXTI_ClearITPendingBit(EXTI_Line17);

  /* Check on the AlarmA falg and on the number of interrupts per Second (60*8) */
  if (RTC_GetITStatus(RTC_IT_ALRA) != RESET)
  {
    //STM_EVAL_LEDOn(LED4);
    alarm_now = 0;
    /* Clear RTC AlarmA Flags */
    RTC_ClearITPendingBit(RTC_IT_ALRA);
    RTC_HandlerFlag = ENABLE;
  }
}
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
//interupt and rtc test OK!
void rtc_init(void)
{
	RTC_DateTypeDef RTC_DateStruct_set,RTC_DateStruct_get;
	RTC_TimeTypeDef RTC_TimeStruct_set,RTC_TimeStruct_get;
	RTC_Configuration();
	
	RTC_TimeStruct_set.RTC_H12 = RTC_H12_AM;
	RTC_TimeStruct_set.RTC_Hours = 10;
	RTC_TimeStruct_set.RTC_Minutes = 45;
	RTC_TimeStruct_set.RTC_Seconds = 0;
	
	RTC_DateStruct_set.RTC_Date = 16;
	RTC_DateStruct_set.RTC_Month = RTC_Month_July;
	RTC_DateStruct_set.RTC_WeekDay = RTC_Weekday_Wednesday;
	RTC_DateStruct_set.RTC_Year = 14;
	RTC_SetTime(RTC_Format_BCD, &RTC_TimeStruct_set);
	RTC_SetDate(RTC_Format_BCD, &RTC_DateStruct_set);
	
	RTC_GetTime(RTC_Format_BCD, &RTC_TimeStruct_get);
	RTC_GetDate(RTC_Format_BCD, &RTC_DateStruct_get);
}
