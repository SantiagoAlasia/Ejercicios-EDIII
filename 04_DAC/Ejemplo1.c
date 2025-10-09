/*
*  @brief   Generar una señal analogica de tipo rampa lineal de 0 a 3v3
*           a una frecuencia de 976[Hz]
*  @note    Utilizar la funcionalidad de MATCH del p1.28 como base
*           de tiempo para la generacion.
*  @note    Esta no es la mejor manera de hacer el ejercicio porq el core es el encargado de
*           pasar el valor de conversion al DAC para q convierta (cada 1us).
*           La mejor forma es aprovechar el DMA.
*/

#include "LPC17xx.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_dac.h"

#define NUM_WAVE_SAMPLES    1024 //Resolucion del DAC

uint32_t WaveForm[NUM_WAVE_SAMPLES];

void cfgGPIO(void);
void cfgTimer(void);
void cfgDAC(void);
void cfgWaveForm(void);

int main(void){

    cfgWaveForm();
    cfgGPIO();
    cfgDAC();
    cfgTimer();    //Importante configurarlo desp para que no dispare la conversion antes q este todo listo

    while(1){}

    return 0;
}

void cfgGPIO(void){
    PINSEL_CFG_Type cfgPinMAT0_CH0;    //Configuracion del p1.28
    PINSEL_CFG_Type cfgPinDAC;         //COnfiguracion del p0.26

    cfgPinMAT0_CH0.Portnum = PINSEL_PORT_1;
    cfgPinMAT0_CH0.Pinnum = PINSEL_PIN_28;
    cfgPinMAT0_CH0.Funcnum = PINSEL_FUNC_3; //Funcionalidad MAT
    cfgPinMAT0_CH0.Pinmode = PINSEL_ṔINMODE_TRISTATE;
    cfgPinMAT0_CH0.OpenDrain = PINSEL_PINMODE_NORMAL;

    cfgPinMAT0_CH0.Portnum = PINSEL_PORT_0;
    cfgPinMAT0_CH0.Pinnum = PINSEL_PIN_26;
    cfgPinMAT0_CH0.Funcnum = PINSEL_FUNC_2; //Funcionalidad DAC
    cfgPinMAT0_CH0.Pinmode = PINSEL_ṔINMODE_TRISTATE;
    cfgPinMAT0_CH0.OpenDrain = PINSEL_PINMODE_NORMAL;

    PINSEL_configPin(&cfgPinMAT0_CH0);
    PINSEL_configPin(&cfgPinDAC);

    return;
}

void cfgTimer(void){
    TIM_TIMERCFG_Type cfgTimerMode;
    TIM_MATCHCFG_Type cfgTimerMatch;

    cfgTimerMode.PrescaleOption = TIM_PRESCALE_USVAL;
    cfgTimerMode.PrescaleValue = 0;

    cfgTimerMatch.MatchChannel = 0; //Habilitacion de canal de MATCH0
    cfgTimerMatch.MatchValue = 1; //Valor maximo para el TC (MR)
    cfgTimerMatch.IntOnMatch = ENABLE; //Habilitacion de Interrupcion por MATCH
    cfgTimerMatch.ResetOnMatch = ENABLE; //Habilitacion de Reset o MATCH
    cfgTimerMatch.StopOnMatch = DISABLE; //Deshabbilitacion de stop por MATCH
    cfgTimerMatch.ExtMatchOutputType = TIM_EXTMATCH_NOTHING; //Modalidad del pin de MATCH (Aunque no lo usemos hay q configurarlo arriba en el cfgGPIO)

    TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &cfgTimerMode); //Configuracion de la modalidad del TMR0
    TIM_ConfigMatch(LPC_TIM0, &confTimerMatch); //Configuracion del TMR0 como temporizador
    TIM_Cmd(LPC_TIM0, ENABLE); //Habilitacion de cuenta de TMR0

    NVIC_EnableIRQ(TIMER0_IRQn);

    return;
}

void cfgDAC(void){
    DAC_Init(LPC_DAC); //Inicializacion del DAC

    return;
}

void cfgWaveForm(void){
    //Generacion de rampa lineal
    for(uint16_t i = 0; i < NUM_WAVE_SAMPLES; i++){
        WaveForm[i] = i;
    }

    return;
}

void TIMER0_IRQHandler(void){
    if(TIM_GetIntStatus(LPC_TIM0, TIM_MR0_INT)){  //Consulta por la interrupcion por MR0
        static uint16_t indexWave = 0;
        DAC_UpdateValue(LPC_DAC, WaveForm[indexWave]); //Actualiza el valor a convertir por el DAC
        indexWave = (indexWave + 1) % NUM_WAVE_SAMPLES; //Incrementa el Buffer Circular

        TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT); //Limpia bandera de interrupcion por TMR0
    }

    return;
}