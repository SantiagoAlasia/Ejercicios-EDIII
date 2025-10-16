/*
*   @brief   Configuracion del ADC para transferencia P2M
*            con LLI simple via DMA
*   @note    El ejercicio NO esta completo, solo es para aprender a usar DMA.
*/

#include "LPC17xx.h"
#include "lpc17xx_gpdma.h"

#define ADC_BUFFER0_START    0x2007C000
#define ADC_BUFFER0_END    0x2007E000
#define ADC_BUFFER0_SIZE    ((ADC_BUFFER0_END - ADC_BUFFER0_START)/sizeof(uint32_t))

volatile uint32_t* adc_samples = (volatile uint32_t *)ADC_BUFFER0_START;
volatile bool ADC_FLAG = false;

void cfgDMA(void);
void storeADCsamples(void);

int main(void){
    cfgDMA();

    while(1){
        if(ADC_FLAG){
            storeADCsamples();
        }
    }

    return 0;
}

void storeADCsamples(void){
    GPDMA_ChannelCmd(LPC_GPDMA, ENABLE);   //Inicializacion de envio de datos por canal 0 de DMA

    return;
}

void cfgDMA(void){
    GPDMA_LLI_Type cfgADC_LLI0_CH0;
    GPDMA_Channel_CFG_Type cfgDMA_CH0;

    NVIC_DisableIRQ(DMA_IRQn);   //Deshabilitacion de la interupcion por DMA.
    GPDMA_Init();   //Inicializacion del controlador GPDMA

    cfgADC_LLI0_CH0.SrcAddr = (uint32_t)&(LPC_ADC->ADDR0); //Direccion de Fuente de datos (Periferico)
    cfgADC_LLI0_CH0.DstAddr = (uint32_t)adc_samples; //Direccion de destino de datos(Memoria)
    cfgADC_LLI0_CH0.NextLLI = (uint32_t)&cfgADC_LLI0_CH0; //Proxima direccion de destino de datos: LLI Repetitiva
    cfgADC_LLI0_CH0.Control = (ADC_BUFFER_SIZE << 0)      //Tamaño de la transferencia de la LLI
                            | (2 << 18)                   //Ancho de la transferencia Fuente (32 bit)
                            | (2 << 21)                   //Ancho de la transferencia Destino (32 bit)
                            | (1 << 27)                   //DI=1: Con incremento de la direccion de destino
                            & ~(1 << 26);                 //SI=0: Sin incremento de la direccion de fuente

    cfgADC_DMA_CH0.ChannelNum = 0;    //Seleccion del canal de transferencia
    cfgADC_DMA_CH0.SrcConn = GPDMA_CONN_ADC;    //Periferico de fuente de datos
    cfgADC_DMA_CH0.DstConn = 0;    //Periferico de destino de datos (Sin Uso)
    cfgADC_DMA_CH0.SrcMemAddr = 0;    //Direccion de fuente de datos en memoria(Sin Uso)
    cfgADC_DMA_CH0.DstMemAddr = (uint32_t)adc_samples;    //Direccion de destino de datos en memoria
    cfgADC_DMA_CH0.TransferType = GPDMA_TRANSFERTYPE_P2M;    //Tipo de transferencia (Periferico a Memoria)
    cfgADC_DMA_CH0.TransferSize = ADC_BUFFER0_SIZE;    //Tamaño de transferencia de datos
    cfgADC_DMA_CH0.TransferWidth 0;    //Ancho de transferencia de datos (Sin Uso)
    cfgADC_DMA_CH0.DMALLI = (uint32_t)&cfgADC_LLI0_CH0;    //Lista enlazada incial

    GPDMA_Setup(&cfgADC_DMA_CH0);    //Configuracion del canal 0 de DMA

    return;
}