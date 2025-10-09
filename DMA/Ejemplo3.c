/*
*    @brief    Configuracion del periferico DAC prar transferencia M2P con LLI multiple e ISR via GPDMA
*/

#include "LPCxx.h"
#include "lpc17xx_gpdma.h"

#define DAC_BUFFER0_START    0x2007C000
#define DAC_BUFFER1_START    0x2007D000
#define DAC_BUFFER2_START    0x2007E000
#define NUM_WAVE_SAMPLES     1024

volatile uint32_t *dac_samples0 = (volatile uint32_t *)DAC_BUFFER0_START;
volatile uint32_t *dac_samples1 = (volatile uint32_t *)DAC_BUFFER1_START;
volatile uint32_t *dac_samples2 = (volatile uint32_t *)DAC_BUFFER2_START;
volatile bool DAC_FLAG = false;
volatile uint32_t DMA_CH0_TC = 0;
volatile uint32_t DMA_CH0_ERR = 0;

void cfgDMA(void);
void loadDACwaves(void);

int main(void){
    cfgDMA();

    while(1){
        if(DAC_FLAG){
            loadDACwaves();
        }
    }

    return 0;
}

void loadDACwave(void){
    GPDMA_ChannelCmd(LPC_GPDMACH0, ENABLE);    //Inicializacion de envio de datos por canal 0 de DMA
    NVIC_EnableIRQ(DMA_IRQn);    //Habilitacion de interrupcion por DMA cuando termina de transferir los datos

    return;
}

void cfgDMA(void){
    GPDMA_LLI_Type cfgDAC_LLI0_CH0;    //Estructura para lista enlazada 0 para canal 0 del DMA
    GPDMA_LLI_Type cfgDAC_LLI1_CH0;    //Estructura para lista enlazada 1 para canal 0 del DMA
    GPDMA_LLI_Type cfgDAC_LLI2_CH0;    //Estructura para lista enlazada 2 para canal 0 del DMA
    GPDMA_Channel_CFG_Type cfgDAC_DMA_CH0;    //Estructura de configuracion para canal 0 del DMA

    NVIC_DisableIRQ(DMA_IRQn);    //Deshabilitacion de interupcion por DMA
    GPDMA_Init();    //Inicializacion del controlador GPDMA

    cfgDAC_LLI0_CH0.SrcAddr = (uint32_t)dac_samples0; //Direccion de Fuente de datos (Memoria)
    cfgDAC_LLI0_CH0.DstAddr = (uint32_t)&(LPC_DAC->DACR); //Direccion de destino de datos(Periferico)
    cfgDAC_LLI0_CH0.NextLLI = (uint32_t)&cfgDAC_LLI1_CH0; //Proxima direccion de destino de datos: LLI Repetitiva
    cfgDAC_LLI0_CH0.Control = (NUM_WAVE_SAMPLES << 0)      //Tamaño de la transferencia de la LLI
                            | (2 << 18)                   //Ancho de la transferencia Fuente (32 bit)
                            | (2 << 21)                   //Ancho de la transferencia Destino (32 bit)
                            | (1 << 26)                   //SI=1: Con incremento de la direccion de Fuente
                            & ~(1 << 27);                 //DI=0: Sin incremento de la direccion de Destino

    cfgDAC_LLI0_CH0.SrcAddr = (uint32_t)dac_samples1; //Direccion de Fuente de datos (Memoria)
    cfgDAC_LLI0_CH0.DstAddr = (uint32_t)&(LPC_DAC->DACR); //Direccion de destino de datos(Periferico)
    cfgDAC_LLI0_CH0.NextLLI = (uint32_t)&cfgDAC_LLI2_CH0; //Proxima direccion de destino de datos: LLI Repetitiva
    cfgDAC_LLI0_CH0.Control = (NUM_WAVE_SAMPLES << 0)      //Tamaño de la transferencia de la LLI
                            | (2 << 18)                   //Ancho de la transferencia Fuente (32 bit)
                            | (2 << 21)                   //Ancho de la transferencia Destino (32 bit)
                            | (1 << 26)                   //SI=1: Con incremento de la direccion de Fuente
                            & ~(1 << 27);                 //DI=0: Sin incremento de la direccion de Destino

    cfgDAC_LLI0_CH0.SrcAddr = (uint32_t)dac_samples0; //Direccion de Fuente de datos (Memoria)
    cfgDAC_LLI0_CH0.DstAddr = (uint32_t)&(LPC_DAC->DACR); //Direccion de destino de datos(Periferico)
    cfgDAC_LLI0_CH0.NextLLI = (uint32_t)&cfgDAC_LLI0_CH0; //Proxima direccion de destino de datos: LLI Repetitiva
    cfgDAC_LLI0_CH0.Control = (NUM_WAVE_SAMPLES << 0)      //Tamaño de la transferencia de la LLI
                            | (2 << 18)                   //Ancho de la transferencia Fuente (32 bit)
                            | (2 << 21)                   //Ancho de la transferencia Destino (32 bit)
                            | (1 << 26)                   //SI=1: Con incremento de la direccion de Fuente
                            & ~(1 << 27);                 //DI=0: Sin incremento de la direccion de Destino


    cfgDAC_DMA_CH0.ChannelNum = 0;    //Seleccion del canal de transferencia
    cfgDAC_DMA_CH0.SrcConn = 0;    //Periferico de fuente de datos (Sin Uso)
    cfgDAC_DMA_CH0.DstConn = GPDMA_CONN_DAC;    //Periferico de destino de datos (DAC)
    cfgDAC_DMA_CH0.SrcMemAddr = cfgDAC_LLI0_CH0.srcAddr;    //Direccion de fuente de datos en memoria
    cfgDAC_DMA_CH0.DstMemAddr = 0;    //Direccion de destino de datos en memoria(Sin Uso)
    cfgDAC_DMA_CH0.TransferType = GPDMA_TRANSFERTYPE_M2P;    //Tipo de transferencia (Memoria a Periferico)
    cfgDAC_DMA_CH0.TransferSize = NUM_WAVE_SAMPLES;    //Tamaño de transferencia de datos
    cfgDAC_DMA_CH0.TransferWidth 0;    //Ancho de transferencia de datos (Sin Uso)
    cfgDAC_DMA_CH0.DMALLI = (uint32_t)&cfgDAC_LLI0_CH0;    //Lista enlazada incial

    GPDMA_Setup(&cfgDAC_DMA_CH0);    //Configuracion del canal 0 de DMA

    return;
}

void DMA_IRQHandler(void){
    if(GPDMA_IntGetStatus(GPDMA_STAT_INT, 0)){
        if(GPDMA_Int_Get_Status(GPDMA_STAT_INTTC, 0)){    //Verifica interrupcion por cuenta terminal
            GPDMA_ClearPending(GPDMA_STATCLR_INTTC, 0);    //Limpia la bandera de interrupcion por cuenta terminal
        }
        if(GPDMA_IntGetStatus(GPDMA_STAT_INTERR, 0)){    //Verifica Interrupcion por error terminal
            GPDMA_ClearPending(GPDMA_STATCLR_INTERR, 7);    //Limpia la bandera de interrupcion por error terminal
        }
    }

    return;
}