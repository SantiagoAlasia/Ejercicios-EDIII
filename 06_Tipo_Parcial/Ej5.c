/*
*    @brief     Detectar el periodo de una señal y determinar el rango de frecuencia en el q se encuentra.
*               En base al valor medido de frencuencia se debe activar el pin adectuado generando una señal con el valor
*               maximo del rango. Para cambiar de rango debo sensar 10 muestras del mismo rango.
*/
#include "LPC17xx.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_timer.h"

volatile uint8_t ultimoRangoFrec = 0;
volatile uint8_t actualFrec = 0;

void cfgGPIO(void);
void cfgTimer0(void);
void cfgTimer1(void);

int main(void){
   cfgGPIO();
   cfgTimer0();
   cfgTimer1();

   while(1){};

   return 0;
}

void cfgGPIO(void){
   PINSEL_CFG_Type pinCfgCap;

   pinCfgCap.Portnum = PINSEL_PORT_1;
   pinCfgCap.Pinnum = PINSEL_PIN_26;
   pinCfgCap.Funcnum = PINSEL_FUNC_3;
   pinCfgCap.Pinmode = PINSEL_PINMODE_TRISTATE;
   pinCfgCap.OpenDrain = PINSEL_PINMODE_NORMAL;

   PINSEL_ConfigPin(&pinCfgCap);

   PINSEL_CFG_Type pinCfgMat;

   pinCfgMat.Portnum = PINSEL_PORT_1;
   pinCfgMat.Pinnum = PINSEL_PIN_22;
   pinCfgMat.Funcnum = PINSEL_FUNC_3;
   pinCfgMat.Pinmode = PINSEL_PINMODE_TRISTATE;
   pinCfgMat.OpenDrain = PINSEL_PINMODE_NORMAL;

   PINSEL_ConfigPin(&pinCfgMat);

   PINSEL_CFG_Type pinCfgPin0;

   pinCfgPin0.Portnum = PINSEL_PORT_0;
   pinCfgPin0.Pinnum = PINSEL_PIN_0;
   pinCfgPin0.Funcnum = PINSEL_FUNC_0;
   pinCfgPin0.Pinmode = PINSEL_PINMODE_TRISTATE;
   pinCfgPin0.OpenDrain = PINSEL_PINMODE_NORMAL;

   PINSEL_ConfigPin(&pinCfgPin0);
   GPIO_SetDir(PORT_0, PIN_0, OUTPUT);

   PINSEL_CFG_Type pinCfgPin1;

   pinCfgPin1.Portnum = PINSEL_PORT_0;
   pinCfgPin1.Pinnum = PINSEL_PIN_1;
   pinCfgPin1.Funcnum = PINSEL_FUNC_0;
   pinCfgPin1.Pinmode = PINSEL_PINMODE_TRISTATE;
   pinCfgPin1.OpenDrain = PINSEL_PINMODE_NORMAL;

   PINSEL_ConfigPin(&pinCfgPin1);
   GPIO_SetDir(PORT_0, PIN_1, OUTPUT);

   PINSEL_CFG_Type pinCfgPin2;

   pinCfgPin2.Portnum = PINSEL_PORT_0;
   pinCfgPin2.Pinnum = PINSEL_PIN_2;
   pinCfgPin2.Funcnum = PINSEL_FUNC_0;
   pinCfgPin2.Pinmode = PINSEL_PINMODE_TRISTATE;
   pinCfgPin2.OpenDrain = PINSEL_PINMODE_NORMAL;

   PINSEL_ConfigPin(&pinCfgPin2);
   GPIO_SetDir(PORT_0, PIN_2, OUTPUT);

   PINSEL_CFG_Type pinCfgPin3;

   pinCfgPin3.Portnum = PINSEL_PORT_0;
   pinCfgPin3.Pinnum = PINSEL_PIN_3;
   pinCfgPin3.Funcnum = PINSEL_FUNC_0;
   pinCfgPin3.Pinmode = PINSEL_PINMODE_TRISTATE;
   pinCfgPin3.OpenDrain = PINSEL_PINMODE_NORMAL;

   PINSEL_ConfigPin(&pinCfgPin3);
   GPIO_SetDir(PORT_0, PIN_3, OUTPUT);

   return;
}

void cfgTimer0(void){
   TIM_COUNTERCFG_Type cfgTimerMode;
   TIM_CAPTURECFG_Type cfgCap;

   cfgTimerMode.CounterOption = TIM_COUNTER_INCAP0;

   cfgCap.CaptureChannel = 0;
   cfgCap.RisingEdge = ENABLE;
   cfgCap.FallingEdge = DISABLE;
   cfgCap.IntOnCaption = ENABLE;

   TIM_Init(LPC_TIM0, TIM_COUNTER_RISING, &cfgTimerMode);
   TIM_ConfigCapture(LPC_TIM0, &cfgCap);
   TIM_Cmd(LPC_TIM0, ENABLE);

   NVIC_EnableIRQ(TIM0_IRQn);

   return;
}

void cfgTimer1(void){
   TIM_TIMERCFG_Type cfgTimerMode;
   TIM_MATCHCFG_Type cfgMat;

   cfgTimerMode.PrescaleOption = TIM_PRESCALE_USVAL;
   cfgTimerMode.PrescaleValue = 25; //Interumpe cada 25us q es el valor minimo por ciclo

   cfgMat.MatchChannel = 0;
   cfgMat.MatchValue = 1;
   cfgMat.IntOnMatch = ENABLE;
   cfgMat.StopOnMatch = DISABLE;
   cfgMat.ResetOnMatch = ENABLE;
   cfgMat.ExtMatchOutput = TIM_EXTMATCH_NOTHING;

   TIM_Init(LPC_TIM0, TIM_TIMER_MPDE, &cfgTimerMode);
   TIM_ConfigMatch(LPC_TIM0, &cfgMat);
   TIM_Cmd(LPC_TIM0, ENABLE);

   NVIC_EnableIRQ(TIM0_IRQn);

   return;
}

void TIM0_IRQHandler(void){
   static uint32_t lastCapValue = 0;
   static uint32_t currCapValue = 0;
   static uint8_t capFlag = 1;
   static uint8_t muestrasCons = 0;

   if(TIM_GetIntStatus(LPC_TIM0, TIM_CR0_INT)){
      currCapValue = TIM_GetCaptureValue(LPC_TIM0, TIM_COUNTER_INCAP0); //Valor del registro de captura CAP0

      if(capFlag){
         capFlag = 0;
      }else{
         uint32_t diffTicks = (currCapValue - lastCapValue);
         float tTicks = 1.0f /25000000.0f;
         float diffTime = tTicks * diffTicks;

         if(diffTime >= 0.0001f && diffTime < 0.0005f){
            if(ultimoRangoFrec == 0){
               muestrasCons++;
            }else{
               ultimoRangoFrec= 0;
               muestrasCons = 1;
            }
         }else if(diffTime >= 0.00005f && diffTime < 0.0001f){
            if(ultimoRangoFrec == 1){
               muestrasCons++;
            }else{
               ultimoRangoFrec = 1;
               muestrasCons = 1;
            }
         }else if(diffTime >= 0.000033f && diffTime < 0.00005f){
            if(ultimoRangoFrec == 2){
               muestrasCons++;
            }else{
               ultimoRangoFrec= 2;
               muestrasCons = 1;
            }
         }else if(diffTime >= 0.000025f && diffTime < 0.000033f){
            if(ultimoRangoFrec == 3){
               muestrasCons++;
            }else{
               ultimoRangoFrec = 3;
               muestrasCons = 1;
            }
         }

         if(muestrasCons >= 10){
            actualFrec = ultimoRangoFrec;
            muestrasCons = 0;
         }
      }
      lastCapValue = currCapValue;

      TIM_ClearIntPending(LPC_TIM0, TIM_CR0_INT);
   }

   return;
}

void TIM1_IRQHandler(void){
   static num = 0;

  if(TIM_GetIntStatus(LPC_TIM1, TIM_MR0_INT)){
     num++;

     switch(actualFrec){
        case 0:
            if(num >= 8){
               // Logica para cambiar el estado
               num = 0;
            }
            break;
     }

     TIM_ClearIntPending(LPC_TIM1, TIM_MR0_INT);
  }

   return;
}