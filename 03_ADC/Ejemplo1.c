/*
 * @brief  Adquirir una señal analogica de 16KHz por el p0.23 cada 1s.
 * @note   Utilizar la funcionalidad de MATCH del p1.29 como base de tiempo para
 *         la adquiscion.
 */

#include "LPC17xx.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_adc.h"

void cfgGPIO(void);
void cfgTimer(void);
void cfgADC(void);

int main(void){
    cfgGPIO();
    cfgTimer();
    cfgADC();

    while(1){}

    return 0;
}

void cfgGPIO(void){
    PINSEL_CFG_Type cfgPinADC0_CH0;    //Configuracion del p0.23
    PINSEL_CFG_Type cfgPinMAT0_CH1;    //Configuracion del p1.29

    cfgPinADC0_CH0.Portnum = PINSEL_PORT_0;
    cfgPinADC0_CH0.Pinnum = PINSEL_PIN_23;
    cfgPinADC0_CH0.Funcnum = PINSEL_FUNC_1; //Funcionalidad ADC
    cfgPinADC0_CH0.Pinmode = PINSEL_PINMODE_TRISTATE; //Pin flotante (Ni pull up, ni pull down)
    cfgPinADC0_CH0.OpenDrain = PINSEL_PINMODE_NORMAL;

    cfgPinMAT0_CH1.Portnum = PINSEL_PORT_1;
    cfgPinMAT0_CH1.Pinnum = PINSEL_PIN_29;
    cfgPinMAT0_CH1.Funcnum = PINSEL_FUNC_3; //Funcionalidad MAT
    ccfgPinMAT0_CH1.OpenDrain = PINSEL_PINMODE_NORMAL;

    PINSEL_configPin(&cfgPinADC0_CH0); //Inicializacion ADC
    PINSEL_configPin(&cfgPinMAT0_CH1); //Inicializacion TMR0

    return;
}

void cfgTimer(void){
    TIM_TIMERCFG_Type cfgTimerMode; //Configuracion de Timer en modo temporizador
    TIM_MATCHCFG_Type cfgTimerMatch; //Configuracion del tiempo de desborde de MATCH

    cfgTimerMode.PrescaleOption = TIM_PRESCALE_USVAL; //Base de timepo del prescaler a partir de valor en us
    cfgTimerMode.PrescaleValue = 1000; //Base de tiempo de desborde de prescaler en us (Time_Overflow_PC)

    cfgTimerMatch.MatchChannel = 1; //Habilitacion de canal de MATCH0
    cfgTimerMatch.MatchValue = 499; //Valor maximo para el TC (MR)
    cfgTimerMatch.IntOnMatch = DISABLE; //Habilitacion de Interrupcion por MATCH
    cfgTimerMatch.ResetOnMatch = ENABLE; //Habilitacion de Reset o MATCH
    cfgTimerMatch.StopOnMatch = DISABLE; //Deshabbilitacion de stop por MATCH
    cfgTimerMatch.ExtMatchOutputType = TIM_EXTMATCH_TOGGLE; //Modalidad del pin de MATCH (Aunque no lo usemos hay q configurarlo arriba en el cfgGPIO)

    TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &cfgTimerMode); //Configuracion de la modalidad del TMR0
    TIM_ConfigMatch(LPC_TIM0, &confTimerMatch); //Configuracion del TMR0 como temporizador
    TIM_Cmd(LPC_TIM0, ENABLE); //Habilitacion de cuenta de TMR0

    return;
}

void cfgADC(void){
    ADC_Init(LPC_ADC, 32000); //Inicializacion del ADC con frecuencia de muestreo f_m[Hz]
    ADC_BurstCmd(LPC_ADC, DISABLE); //Deshabilitacion del modo Burst para ADC
    ADC_StartCmd(LPC_ADC, ADC_START_ON_MAT01); //Inicializacion de conversion debiado a MAT0.1
    ADC_EdgeStasrtConfig(LPC_ADC, ADC_START_ON_FALLING); //Conversion ante flanco descendente del pin correspondiente al MAT0.1

    ADC_ChannelCmd(LPC_ADC, ADC_CHANNEL_0, ENABLE); //Funcionalidad del p0.23 como ADC
    ADC_IntConfig(LPC_ADC, ADC_ADINTERN0, ENABLE); //Habilitacion de interrupcion ante finaizacion de convercion por el ADC0

    NVIC_EnableIRQ(ADC_IRQn);

    return;
}

void ADC_IRQHandler(void){
    while(!(ADC_ChannleGetStatus(LPC_ADC, ADC_CHANNEL_0, ADC_DATA_DONE))){}; //Espera fin de conversion de ADC0

    uint16_t ADCValue = ADC_ChannelGetData(LPC_ADC, ADC_CHANNEL_0); //Guarda valor digitalizado de ADC0

    return;
}