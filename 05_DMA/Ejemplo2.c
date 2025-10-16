/*
*    @brief    Configuracion del periferico DAC para transferencia M2P con LLI Simple e ISR via GPDMA
*/

#include "LPC17xx.h"
#include "lpc17xx_gpdma.h"

#define DAC_BUFFER_START    0x20080000    //Primera direccion de la WaveForm
#define NUM_WAVE_SAMPLES    382
volatile uint32_t *dac_samples = (volatile uint32_t *)DAC_BUFFER_START;
volatile bool DAC_FLAG = false;
volatile uint32_t DMA_CH0_TC = 0;
volatile uint32_t DMA_CH0_ERR = 0;

void cfgDMA(void);
void loadDACwave(void);

int main(void){
    cfgDMA();

    while(1){
        if(DAC_FLAG){
            loadDACwave();
        }
    }
    return 0;
}
void loadDACwave(void){
    GPDMA_ChannelCmd(LPC_GPDMACH0, ENABLE);    //Inicializacion de envio de datos por canal 0 de DMA
    NVIC_EnableIRQ(DMA_IRQn);    //Habilitacion de interrupcion por DMA cuando termina de transferir los datos
    return;
}
void cfgDAC(void){
    uint32_t counDAC = 65535;    //Numero de cuentas para el timer dedicado del DAC
    DAC_CONVERTER_CFG_Type cfgDAC;

    cfgDAC.CNT_ENA = SET;    //Habilitacion de TIMER dedicado al DAC
    cfgDAC.DMA_ENA = SET;    //Habilitacion de DAC por DMA

    DAC_Init(LPC_DAC);    //Inicializacion del DAC

    DAC_SetBias(LPC_DAC, DAC_MAX_CURRENT_350uA);

    DAC_ConfigDAConverterControl(LPC_DAC, &cfgDAC);    //Configuracion del DAC
    DAC_SetDMATimeOut(LPC_DAC, countDAC);    //Establecimiento del valor de recarga para el timer dedicado
    return;
}
void cfgDMA(void){
    GPDMA_LLI_Type cfgDAC_LLI0_CH0;    //Estructura para lista enlazada para canal 0 del DMA
    GPDMA_Channel_CFG_Type cfgDAC_DMA_CH0;    //Estructura de configuracion para canal 0 del DMA

    NVIC_DisableIRQ(DMA_IRQn);    //Deshabilitacion de interupcion por DMA
    GPDMA_Init();    //Inicializacion del controlador GPDMA

    cfgDAC_LLI0_CH0.SrcAddr = (uint32_t)dac_samples; //Direccion de Fuente de datos (Memoria)
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
    cfgDAC_DMA_CH0.SrcMemAddr = (uint32_t)dac_samples;    //Direccion de fuente de datos en memoria
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