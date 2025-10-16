/*
*    @brief    Usando CAP debemos desmodular una señal PWM y sacar una tension por el DAC proporcional al
*              ciclo de trabajo. Este valor debe ser entre 0v y 2v con una frecuencia de actualizacion de 0,5s.
*              Usar el ciclo de trabajo promedio de las ultimas 10 muestras.
*/

#include "LPC17xx.h"
#include "lpc17xx_pinsel.h"
#include "lpcc17xx_gpio.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_dac.h"

#define MUESTRAS 10
#define VALOR_2V_DAC 620      // Valor DAC para 2V .  (2*1023)/3.3=620

volatile uint32_t capturaAnterior = 0;
volatile uint32_t capturaInicial = 0;
volatile uint32_t periodo = 0;
volatile uint32_t CICLO_TRABAJO = 0;
volatile uint8_t flancoSubida = 1;
volatile uint32_t CAPTURAS_DE_CICLOS_TRABAJO[MUESTRAS];
volatile uint8_t INDICE_CAPTURAS = 0;

void cfgGPIO(void);
void cfgTimer0(void);
void cfgTimer1(void);
void cfgDAC(void);

int main(void){
    cfgGPIO();
    cfgTimer0();
    cfgTimer1();
    cfgDAC();

    while(1){};

    return 0;
}

void cfgGPIO(void){
    PINSEL_CFG_Type pinCfgTimer0;

    pinCfgTimer0.Portnum = 1;
    pinCfgTimer0.Pinnum = 26;
    pinCfgTimer0.Funcnum = 3;
    pinCfgTimer0.Pinmode = PINSEL_PINMODE_TRISTATE;
    pinCfgTimer0.OpenDrain = PINSEL_PINMODE_NORMAL;

    PINSEL_ConfigPin(&pinCfgTimer0);

    PINSEL_CFG_Type pinCfgTimer1;

    pinCfgTimer1.Portnum = 1;
    pinCfgTimer1.Pinnum = 22;
    pinCfgTimer1.Funcnum = 3;
    pinCfgTimer1.Pinmode = PINSEL_PINMODE_TRISTATE;
    pinCfgTimer1.OpenDrain = PINSEL_PINMODE_NORMAL;

    PINSEL_ConfigPin(&pinCfgTimer1);

    PINSEL_CFG_Type pinCfgDAC;

    pinCfgDAC.Portnum = 0;
    pinCfgDAC.Pinnum = 26;
    pinCfgDAC.Funcnum = 2;
    pinCfgDAC.Pinmode = PINSEL_PINMODE_TRISTATE;
    pinCfgDAC.OpenDrain = PINSEL_PINMODE_NORMAL;

    PINSEL_ConfigPin(&pinCfgDAC);

    return;
}

void cfgTimer0(void){
    TIM_COUNTERCFG_Type cfgTimerMode;
    TIM_CAPTURECFG_Type cfgTimerCap;

    cfgTimerMode.CounterOption = TIM_COUNTER_INCAP0;

    cfgTimerCap.CaptureChannel = 0;
    cfgTimerCap.RisingEdge = ENABLE;
    cfgTimerCap.FallingEdge = ENABLE;
    cfgTimerCap.IntOnCaption = ENABLE;

    TIM_Init(LPC_TIM0, TIM_COUNTER_ANY, &cfgTimerMode);
    TIM_ConfigCapture(LPC_TIM0, &cfgTimerCap);
    TIM_Cmd(LPC_TIM0, ENABLE);

    NVIC_EnableIRQ(TIM0_IRQn);

    return;
}

void cfgTimer1(void){
    TIM_TIMERCFG_Type cfgTimerMode;
    TIM_MATCHCFG_Type cfgTimerMat;

    cfgTimerMode.PrescaleOption = TIM_PRESCALE_USVAL;
    cfgTimerMode.PrescaleValue = 100000;

    cfgTimerMat.MatchChannel = 0;
    cfgTimerMat.MatchValue = 5;
    cfgTimerMat.IntOnMatch = ENABLE;
    cfgTimerMat.StopOnMatch = DISABLE;
    cfgTimerMat.ResetOnMatch = ENABLE;
    cfgTimerMat.ExtMatchOutputType = TIM_EXTMATCH_NOTHING;

    TIM_Init(LPC_TIM1, TIM_TIMER_MODE, &cfgTimerMode); //Configuracion de la modalidad del TMR0
    TIM_ConfigMatch(LPC_TIM1, &confTimerMatch); //Configuracion del TMR0 como temporizador
    TIM_Cmd(LPC_TIM1, ENABLE); //Habilitacion de cuenta de TMR0

    NVIC_EnableIRQ(TIM1_IRQn);

    return;
}

void cfgDAC(void){
    DAC_Init(LPC_DAC);
    DAC_SetBias(LPC_DAC, DAC_700uA);

    return;
}

void TIM0_IRQHandler(void){
    static uint32_t capturaActual;
    if(TIM_GetIntCaptureStatus(LPC_TIM0, 0)) {
        capturaActual = TIM_GetCaptureValue(LPC_TIM0, 0);
        if(flancoSubida){
            periodo = capturaActual - capturaInicial;
            capturaInicial = capturaActual;
            flancoSubida = 0;
        }
        else{
            CICLO_TRABAJO = capturaActual - capturaAnterior;
            // Calcular y guardar ciclo de trabajo en porcentaje
            CAPTURAS_DE_CICLOS_TRABAJO[INDICE_CAPTURAS] = (CICLO_TRABAJO * 100) / periodo;
            INDICE_CAPTURAS = (INDICE_CAPTURAS + 1) % MUESTRAS;
            flancoSubida = 1;
        }

        capturaAnterior = capturaActual;
        TIM_ClearIntCapturePending(LPC_TIM0, 0);
    }

    return;
}

void TIMER1_IRQHandler(void) {
    if(TIM_GetIntStatus(LPC_TIM1, TIM_MR0_INT)) {
        // Calcular promedio de los últimos 10 valores
        uint32_t suma = 0;
        for(int i = 0; i < MUESTRAS; i++) {
            suma += CAPTURAS_DE_CICLOS_TRABAJO[i];
        }
        uint32_t promedio = suma / MUESTRAS;
        // Convertir promedio  a valor DAC entre 0-620 para 0-2V
        uint32_t valorDAC = (promedio * VALOR_2V_DAC) / 100;
        DAC_UpdateValue(valorDAC);

        TIM_ClearIntPending(LPC_TIM1, TIM_MR0_INT);
    }
}