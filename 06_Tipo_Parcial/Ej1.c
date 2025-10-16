/*
* @brief Se lee el valor de un sensor analogico lineal 0-100 [°C].
*        La conversion debe iniciar al transcurrir 100ms, utilizando TIMER 0. (MAT0.1)
*        Dependiendo del valor analogico sensado se deben prender los siguientes led:
*            1. Led Verde -> 0-40 °C
*            2. Led Amarillo -> 41-60°C
*            3. Led Rojo -> 61-100°C
*        Obs: El color rojo solo se habilita con 10 muestras consecutivas en el rango especifico.
* @note No es nesesario almacenar el valor de la conversion, simplemente realizar una accion en base al valor actual.
*/

#include "LPC17xx.h"
#include "lpc17xx_gpio.h""
#include "lpc17xx_pinsel.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_adc.h""

void cfgGPIO(void);
void cfgADC(void);
void cfgTimer(void);

int main(void){
    cfgGPIO();
    cfgADC();
    cfgTimer();

    while(1){};

    return 0;
}

void cfgGPIO(void){
    PINSEL_CFG_Type pinCfgMAT01;    //Pin del MAT0.1

    pinCfgMAT01.Portnum = PINSEL_PORT_1;
    pinCfgMAT01.Pinnum = PINSEL_PIN_29;
    pinCfgMAT01.Funcnum = PINSEL_FUNC_3;
    pinCfgMAT01.Pinmode = PINSEL_PINMODE_TRISTATE;
    pinCfgMAT01.OpenDrain = PINSEL_PINMODE_NORMAL;

    PINSEL_ConfigPin(&pinCfgMAT01);

    PINSEL_CFG_Type pinCfgADC;    //Pin asignado al ADC

    pinCfgADC.Portnum = PINSEL_PORT_0;
    pinCfgADC.Pinnum = PINSEL_PIN_23;
    pinCfgADC.Funcnum = PINSEL_FUNC_1;
    pinCfgADC.Pinmode = PINSEL_PINMODE_TRISTATE;
    pinCfgADC.OpenDrain = PINSEL_PINMODE_NORMAL;

    PINSEL_ConfigPin(&pinCfgADC);

    PINSEL_CFG_Type pinCfgLedV;    //Pin asignado al led verde

    pinCfgLedV.Portnum = PINSEL_PORT_0;
    pinCfgLedV.Pinnum = PINSEL_PIN_22;
    pinCfgLedV.Funcnum = PINSEL_FUNC_3;
    pinCfgLedV.Pinmode = PINSEL_PINMODE_TRISTATE;
    pinCfgLedV.OpenDrain = PINSEL_PINMODE_NORMAL;

    PINSEL_ConfigPin(&pinCfgLedV);

    PINSEL_CFG_Type pinCfgLedA;    //Pin asignado al led amarillo

    pinCfgLedA.Portnum = PINSEL_PORT_0;
    pinCfgLedA.Pinnum = PINSEL_PIN_24; //No es el num de pin real; es a modo de ejemplo.
    pinCfgLedA.Funcnum = PINSEL_FUNC_3;
    pinCfgLedA.Pinmode = PINSEL_PINMODE_TRISTATE;
    pinCfgLedA.OpenDrain = PINSEL_PINMODE_NORMAL;

    PINSEL_ConfigPin(&pinCfgLedA);

    // PINSEL_CFG_Type pinCfgLedR;    //Pin asignado al led rojo - Misma configuracion que arriba

    // Config como salida de los pines para los LEDS.
    GPIO_SetDir(PORT_0, PIN_22, OUTPUT);
    GPIO_SetDir(PORT_0, PIN_24, OUTPUT);
    GPIO_SetDir(PORT_0, PIN_25, OUTPUT);

    return;
}

void cfgADC(void){
    ADC_Init(LPC_ADC, 32000);
    ADC_BurstCmd(LPC_ADC, DISABLE);
    ADC_StartCmd(LPC_ADC, ADC_START_ON_MAT_01);
    ADC_EdgeStartConfig(LPC_ADC, ADC_START_ON_FALLING);
    ADC_ChannelCmd(LPC_ADC, ADC_CHANNEL_0, ENABLE);
    ADC_IntConfig(LPC_ADC, ADC_ADINTEN0, ENABLE);

    NVIC_EnableIRQ(ADC_IRQn);

    return;
}

void cfgTimer(void){
    TIM_TIMERCFG_Type cfgTimerMode;
    TIM_MATCHCFG_Type cfgTimerMatch;

    cfgTimerMode.PrescaleOption = TIM_PRESCALE_USVAL;
    cfgTimerMode.PrescaleValue = 1000; //Tiempo de desborde, 1000us = 1ms

    cfgTimerMatch.MatchChannel = 1;
    cfgTimerMatch.MatchValue = 49;
    cfgTimerMatch.IntOnMatch = DISABLE;
    cfgTimerMatch.StopOnMatch = DISABLE;
    cfgTimerMatch.ResetOnMatch = ENABLE;
    cfgTimerMatch.ExtMatchOutput = TIM_EXTMATCH_TOGGLE;

    TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &cfgTimerMode);
    TIM_CongigMatch(LPC_TIM0, &cfgTimerMatch);
    TIM_Cmd(LPC_TIM0, ENABLE);

    return;
}

void ADC_IRQHandler(void){
    static uint8_t numMuestra = 0;

    while(!ADC_ChannelGetStatus(LPC_ADC, ADC_CHANNEL_0, ADC_DATA_DONE)) {};

    uint16_t adc_value = ADC_ChannelGetData(LPC_ADC, ADC_CHANNEL_0);

    if (adc_value > 410){
        if (adc_value > 614){
            numMuestras++;
            if(numMuestras >= 10){
                numMuestras = 0;
                GPIO_ClearValue(PORT_0, PIN_22);
                GPIO_ClearValue(PORT_0, PIN_24);
                GPIO_SetValue(PORT_0, PIN_25);
            }else{
                GPIO_ClearValue(PORT_0, PIN_22);
                GPIO_SetValue(PORT_0, PIN_24);
                GPIO_ClearValue(PORT_0, PIN_25);
            }
        }else{
            GPIO_ClearValue(PORT_0, PIN_22);
            GPIO_SetValue(PORT_0, PIN_24);
            GPIO_ClearValue(PORT_0, PIN_25);
        }
    }else{
        GPIO_SetValue(PORT_0, PIN_22);
        GPIO_ClearValue(PORT_0, PIN_24);
        GPIO_ClearValue(PORT_0, PIN_25);
    }

    return;
}