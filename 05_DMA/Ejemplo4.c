/*
*    @brief    Configuracion del modulo GPDMA para transferencia M2M sin LLI
*/

#include "LPC17xx.h"
#include "lpc17xx_gpdma.h"

#define MEM_BUFFER_SRC_START    0x2007C000
#define MEM_BUFFER_DST_START    0x2007E000
#define MEM_BUFFER_SIZE    ((MEM_BUFFER_DST_START - MEM_BUFFER_SRC_START)/sizeof(uint32_t))

uint32_t *Fuente = (volatile uint32_t *)MEM_BUFFER_SRC_START;
uint32_t *Destino = (volatile uint32_t *)MEM_BUFFER_DST_START;
volatile bool TRANSF_FLAG = false;

void cfgDMA(void);
void transferStoreData(void);

int main(void){
    cfgDMA();

    while(1){
        if(TRANSFER_FLAG){
            transferStoreData();
        }
    }

    return 0;
}

void transferStoreData(void){
    GPDMA_ChannelCmd(LPC_GPDMACH7, ENABLE);
    NVIC_EnableIRQ(DMA_IRQn);
    return;
}

void cfgDMA(void){
    GPDMA_Channel_CFG_Type cfgMEM_DMA_CH7;    //Estructura de configuracion para canal 7 del DMA

    NVIC_DisableIRQ(DMA_IRQn);    //Deshabilitacion de interupcion por DMA
    GPDMA_Init();    //Inicializacion del controlador GPDMA

    cfgMEM_DMA_CH7.ChannelNum = 7;    //Seleccion del canal de transferencia
    cfgMEM_DMA_CH7.SrcConn = 0;    //Periferico de fuente de datos (Sin Uso)
    cfgMEM_DMA_CH7.DstConn = 0;    //Periferico de destino de datos (Sin Uso)
    cfgMEM_DMA_CH7.SrcMemAddr = (uint32_t)Fuente;    //Direccion de fuente de datos en memoria
    cfgMEM_DMA_CH7.DstMemAddr = (uint32_t)Destino;    //Direccion de destino de datos en memoria
    cfgMEM_DMA_CH7.TransferType = GPDMA_TRANSFERTYPE_M2M;    //Tipo de transferencia (Memoria a Memoria)
    cfgMEM_DMA_CH7.TransferSize = MEM_BUFFER_SIZE;    //Tamaño de transferencia de datos
    cfgMEM_DMA_CH7.TransferWidth GPDMA_WIDTH_WORD;    //Ancho de transferencia de datos (Sin Uso)
    cfgMEM_DMA_CH7.DMALLI = 0;    //Lista enlazada incial

    GPDMA_Setup(&cfgMEM_DMA_CH7);    //Configuracion del canal 0 de DMA

    return;
}

void DMA_IRQHandler(void){
    if(GPDMA_IntGetStatus(GPDMA_STAT_INT, 7)){
        if(GPDMA_Int_Get_Status(GPDMA_STAT_INTTC, 7)){    //Verifica interrupcion por cuenta terminal
            GPDMA_ClearPending(GPDMA_STATCLR_INTTC, 7);    //Limpia la bandera de interrupcion por cuenta terminal
        }
        if(GPDMA_IntGetStatus(GPDMA_STAT_INTERR, 7)){    //Verifica Interrupcion por error terminal
            GPDMA_ClearPending(GPDMA_STATCLR_INTERR, 7);    //Limpia la bandera de interrupcion por error terminal
        }
    }

    return;
}