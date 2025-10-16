#include "LPC17xx.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_exti.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_dac.h"
#include "lpc17xx_dma.h"

#define ADC_BUFFER_START    0x2007C000
#define ADC_BUFFER1_START    0x20080000
#define ADC_BUFFER0_SIZE    ((ADC_BUFFER1_START - ADC_BUFFER0_START)/sizeof(uint32_t))

enum estados {
    ADC_POINTER,
    ADC_DMA,
    DMA_MEM,
    DAC_DATA,
    DAC_WAVE
};

volatile uint8_t index = 0;
volatile estados estado = 0;
volatile uint32_t* adc_muestras = (volatile uint32_t *)ADC_BUFFER0_START;

void cfgGPIO(void);
void cfgEXT(void);
void cfgADC(void);
void cfgTimer(void);

int main(void){
    cfgGPIO();
    cfgEXT();
    cfgADC();
    cfgDMA();
    cfgTimer();

    while(1){}

    return 0;
}

void cfgGPIO(void){
    //Pin de seleccion de modo p2.10
    PINSEL_CFG_Type pinCfgInt;

    pinCfgInt.Portnum = PINSEL_PORT_2;
    pinCfgInt.Pinnum = PINSEL_PIN_10;
    pinCfgInt.Funcnum = PINSEL_FUNC_1;
    pinCfgInt.Pinmode = PINSEL_PINMODE_PULLDOWN;
    pinCfgInt.OpenDrain = PINSEL_OPENDRAIN_NORMAL;

    PINSEL_ConfigPin(&pinCfgInt);

    //Configuracion del p1.28 MAT0
    PINSEL_CFG_Type cfgPinMAT0;

    cfgPinMAT0.Portnum = PINSEL_PORT_1;
    cfgPinMAT0.Pinnum = PINSEL_PIN_28;
    cfgPinMAT0.Funcnum = PINSEL_FUNC_3; //Funcionalidad MAT
    cfgPinMAT0.OpenDrain = PINSEL_PINMODE_NORMAL;

    PINSEL_configPin(&cfgPinMAT0);

    return;
}

/*
* @brief configuracion de la interrupcion EXTI0 por flanco ascendente
* */
void cfgEXT(void){
    EXTI_InitTypeDef EXTI0;

    EXTI0.EXTI_Line = EXTI_LINE_0;
    EXTI0.EXTI_Mode = EXTI_MODE_EDGE_SENSITIVE;
    EXTI0.EXTI_Polarity = EXTI_POLARITY_HIGH_ACTIVE_OR_RISING_EDGE;

    EXTI_Init();
    EXTI_Config(&EXTI0);
    NVIC_EnableIRQ(EXTI0_IRQn);

    return;
}

void EINT0_IRQHandler(void){
    index = (index + 1) % 5;

    switch(index){
        case 0:
            estado = ADC_POINTER;
            cfgADC();
            cfgDMA();
            break;
        case 1:
            estado = ADC_DMA;
            cfgADC();
            cfgDMA();
            break;
        case 2:
            estado = DMA_MEM;
            cfgDMA();
            break;
        default:
            break;
    }

    EXTI_ClearEXTIFlag(EXTI_EINT0);

    return;
}

/*
* @brief configuracion del timer en modo match para q interrumpa cada 1s
* */
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
    ADC_Init(LPC_ADC, 160000); //Inicializacion del ADC con frecuencia de muestreo f_m[Hz]
    ADC_BurstCmd(LPC_ADC, DISABLE); //Deshabilitacion del modo Burst para ADC
    ADC_StartCmd(LPC_ADC, ADC_START_ON_MAT01); //Inicializacion de conversion debiado a MAT0.1
    ADC_EdgeStasrtConfig(LPC_ADC, ADC_START_ON_FALLING); //Conversion ante flanco descendente del pin correspondiente al MAT0.1

    ADC_ChannelCmd(LPC_ADC, ADC_CHANNEL_0, ENABLE); //Funcionalidad del p0.23 como ADC

    if(estado == ADC_POINTER){
        ADC_IntConfig(LPC_ADC, ADC_ADINTERN0, ENABLE); //Habilitacion de interrupcion ante finaizacion de convercion por el ADC0
    }else{
        ADC_IntConfig(LPC_ADC, ADC_ADINTERN0, DISABLE); //Deshabilitacion de interrupcion ante finaizacion de convercion por el ADC0
    }

    NVIC_EnableIRQ(ADC_IRQn);

    return;
}

/*
* @brief Handler del ADC, se llama luego de cada finalizacion de conversion. Solo se usa en estado ADC_POINTER
* */
void ADC_IRQHandler(void){
    while(!(ADC_ChannleGetStatus(LPC_ADC, ADC_CHANNEL_0, ADC_DATA_DONE))){}; //Espera fin de conversion de ADC0

    *adc_muestras = ADC_ChannelGetData(LPC_ADC, ADC_CHANNEL_0); //Guarda valor digitalizado de ADC0

    adc_muestras++;
    if(adc_muestras == ADC_BUFFER1_START){
        adc_muestras = ADC_BUFFER0_START;
    }

    return;
}

void cfgDMA(void){
    GPDMA_Channel_CFG_Type cfgDMA_CH0;
    GPDMA_LLI_Type cfgADC_LLI0_CH0;

    NVIC_DisableIRQ(DMA_IRQn);   //Deshabilitacion de la interupcion por DMA.
    GPDMA_Init();   //Inicializacion del controlador GPDMA

    cfgADC_LLI0_CH0.SrcAddr = (uint32_t)&(LPC_ADC->ADDR0); //Direccion de Fuente de datos (Periferico)
    cfgADC_LLI0_CH0.DstAddr = (uint32_t)adc_muestras; //Direccion de destino de datos(Memoria)
    cfgADC_LLI0_CH0.NextLLI = (uint32_t)&cfgADC_LLI0_CH0; //Proxima direccion de destino de datos: LLI Repetitiva
    cfgADC_LLI0_CH0.Control = (ADC_BUFFER_SIZE << 0)      //Tamaño de la transferencia de la LLI
                            | (2 << 18)                   //Ancho de la transferencia Fuente (32 bit)
                            | (2 << 21)                   //Ancho de la transferencia Destino (32 bit)
                            | (1 << 27)                   //DI=0: Sin incremento de la direccion de destino
                            & ~(1 << 26);                 //SI=1: Con incremento de la direccion de fuente

    cfgADC_DMA_CH0.ChannelNum = 0;    //Seleccion del canal de transferencia
    cfgADC_DMA_CH0.SrcConn = GPDMA_CONN_ADC;    //Periferico de fuente de datos
    cfgADC_DMA_CH0.DstConn = 0;    //Periferico de destino de datos (Sin Uso)
    cfgADC_DMA_CH0.SrcMemAddr = 0;    //Direccion de fuente de datos en memoria(Sin Uso)
    cfgADC_DMA_CH0.DstMemAddr = (uint32_t)adc_muestras;    //Direccion de destino de datos en memoria
    cfgADC_DMA_CH0.TransferType = GPDMA_TRANSFERTYPE_P2M;    //Tipo de transferencia (Periferico a Memoria)
    cfgADC_DMA_CH0.TransferSize = ADC_BUFFER0_SIZE;    //Tamaño de transferencia de datos
    cfgADC_DMA_CH0.TransferWidth 0;    //Ancho de transferencia de datos (Sin Uso)
    cfgADC_DMA_CH0.DMALLI = (uint32_t)&cfgADC_LLI0_CH0;    //Lista enlazada incial

    GPDMA_Setup(&cfgADC_DMA_CH0);    //Configuracion del canal 0 de DMA

    return;
}