/*
Programar el microcontrolador LPC1769 para que mediante su ADC digitalice dos señales analógicas cuyos anchos
de bandas son de 10 Khz cada una. Los canales utilizados deben ser el 2 y el 4 y los datos deben ser guardados
en dos regiones de memorias distintas que permitan contar con los últimos 20 datos de cada canal.
Suponer una frecuencia de core cclk de 100 Mhz. El código debe estar debidamente comentado.
*/

#include "LPC17xx.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_gpdma.h"
#include "lpc17xx_pinsel.h"

void cfgGPIO(void);
void cfgADC(void);
void cfgDMA(void);

#define MUESTRAS 20
#define ADC_BUFFER2_START    0x2007C000
#define ADC_BUFFER4_END    0x2007C014
#define ADC_CH2_ADDR ((uint32_t)&(LPC_ADC->ADDR2))
#define ADC_CH4_ADDR ((uint32_t)&(LPC_ADC ->ADDR4))
volatile uint32_t* buffer_2 = (volatile uint32_t *)ADC_BUFFER2_START;
uint32_t buffer_4[MUESTRAS];  // Para el canal 4 del ADC

int main(void){
    cfgGPIO();
    cfgADC();
    cfgDMA();
    while(1){};
    return 0;
}
void cfgGPIO(void){
    /*canal 2: P0.25
      canal 4: P1.30*/
    PINSEL_CFG_Type pinCfgADC2;

    pinCfgADC2.portNum = PINSEL_PORT_0;
    pinCfgADC2.pinNum  = PINSEL_PIN_25;
    pinCfgADC2.funcNum = PINSEL_FUNC_1;
    pinCfgADC2.pinMode = PINSEL_PINMODE_TRISTATE;
    pinCfgADC2.OpenDrain = PINSEL_PINMODE_NORMAL;

    PINSEL_ConfigPin(&pinCfgADC2);

    PINSEL_CFG_Type pinCfgADC4;

    pinCfgADC4.portNum = PINSEL_PORT_1;
    pinCfgADC4.pinNum  = PINSEL_PIN_30;
    pinCfgADC4.funcNum = PINSEL_FUNC_3;
    pinCfgADC4.pinMode = PINSEL_PINMODE_TRISTATE;
    pinCfgADC4.openDrain = PINSEL_PINMODE_NORMAL;

    PINSEL_ConfigPin(&pinCfgADC4);
    return;
}
void cfgADC(void){
    ADC_Init(LPC_ADC, 30000); // fs = 30kHZ (30 muestras por segundo)
    ADC_BurstCmd(ENABLE);
    ADC_StartCmd(LPC_ADC, ADC_START_CONTINUOS);

    ADC_ChannelCmd(ADC_CHANNEL_2, ENABLE);
    ADC_ChannelCmd(ADC_CHANNEL_4, ENABLE);

    ADC_IntConfig(LPC_ADC, ADC_ADINT2, DISABLE);
    ADC_IntConfig(LPC_ADC, ADC_ADINT4, DISABLE);
    return;
}
void cfgDMA(void){
    /*Transferencia P2M*/
    NVIC_DiableIRQn(ADC_IRQn);
    GPDMA_Init();

    GPDMA_Channel_CFG_Type cfgDMA_2;
    GPDMA_Channel_CFG_Type cfgDMA_4;

    GPDMA_LLI_Type DMA_2_LLI0;
    GPDMA_LLI_Type DMA_4_LLI0;

    //Canal 2
    DMA_2_LLI0.SrcAddr = (uint32_t)&(LPC_ADC->ADDR2);
    DMA_2_LLI0.DstAddr = (uint32_t)buffer_2;
    DMA_2_LLI0.NextLLI = (uint32_t)&DMA_2_LLI0; // Circular
    DMA_2_LLI0.Control = (MUESTRAS)|
                   (2 << 18)| // Transfer width (src)
                   (2 << 21)& // Transfer width (dst)
                   ~(0<<26) | // SI -> NO incremento fuente
                   (1<<27) | // DI -> SI incremento destino
                   (1<<31); // Terminal Count Interrupt Enable

    cfgDMA_2.ChannelNum = 0;
    cfgDMA_2.TransferSize = MUESTRAS;
    cfgDMA_2.TransferWidth = GPDMA_WIDTH_WORD;
    cfgDMA_2.SrcMemAddr = 0;
    cfgDMA_2.DstMemAddr = (uint32_t)buffer_2;
    cfgDMA_2.TransferType = GPDMA_TRANSFERTYPE_P2M;
    cfgDMA_2.SrcConn = GPDMA_CONN_ADC;
    cfgDMA_2.DstConn = 0;
    cfgDMA_2.DMALLI =(uint32_t)&DMA_2_LLI0;

    GPDMA_Setup(&cfgDMA_2);
    GPDMA_ChannelCmd(0, ENABLE);

    //Canal 4
    DMA_4_LLI0.SrcAddr = (uint32_t)&(LPC_ADC->ADDR4);
    DMA_4_LLI0.DstAddr = (uint32_t)buffer_4;
    DMA_4_LLI0.NextLLI = (uint32_t)&DMA_4_LLI0; // Circular
    DMA_2_LLI0.Control = (MUESTRAS)|
                   (2 << 18)| // Transfer width (src)
                   (2 << 21)& // Transfer width (dst)
                   ~(0<<26) | // SI -> NO incremento fuente
                   (1<<27) | // DI -> SI incremento destino
                   (1<<31); // Terminal Count Interrupt Enable

    cfgDMA_4.ChannelNum = 1;
    cfgDMA_4.TransferSize = MUESTRAS;
    cfgDMA_4.TransferWidth = GPDMA_WIDTH_WORD;
    cfgDMA_4.SrcMemAddr = 0;
    cfgDMA_4.DstMemAddr = (uint32_t)buffer_4;
    cfgDMA_4.TransferType = GPDMA_TRANSFERTYPE_P2M;
    cfgDMA_4.SrcConn = GPDMA_CONN_ADC;
    cfgDMA_4.DstConn = 0;
    cfgDMA_4.DMALLI =(uint32_t)&DMA_4_LLI0;

    GPDMA_Setup (&cfgDMA_4);
    GPDMA_ChannelCmd (1, ENABLE);
    return;
}
void DMA_IRQHandler(void){

    volatile uint8_t flag_2 = 0;
    volatile uint8_t flag_4 = 0;

    /*Canal 0: ADC del canal 2*/
    if (GPDMA_IntGetStatus(GPDMA_STAT_INTTC, 0)) /*El canal termino de transferir
    la cantidad total de datos que se le pidio*/ {
        GPDMA_ClearIntPending (GPDMA_STATCLR_INTTC, 0); // Limpia el flag
     flag_2 = 1;
    }

    /*Canal 1: ADC del canal 4*/
    if(GPDMA_IntGetStatus(GPDMA_STAT_INTTC, 1)){
        GPDMA_ClearIntPending (GPDMA_STATCLR_INTTC,1); // Limpia el flag
        flag_4 = 1;
    }

    /* En caso de error */
    if (GPDMA_IntGetStatus(GPDMA_STAT_INTERR, 0)
    || (GPDMA_IntGetStatus (GPDMA_STAT_INTERR,1))) {
        GPDMA_ClearIntPending(GPDMA_STATCLR_INTERR, 0);
        GPDMA_ClearIntPending(GPDMA_STATCLR_INTERR, 1);
    }
}
/*
uint32_t suma = 0;
uint32_t promedio = 0;
for(int i = 0; i < MUESTRAS; i++) {
    suma += CAPTURAS_DE_CICLOS_TRABAJO[i];
}
uint32_t promedio = suma / MUESTRAS;
// Convertir promedio a valor DAC entre 0-620 para 0-2V
uint32_t valorDAC = (promedio * VALOR_2V_DAC) / 100;
DAC_UpdateValue(valorDAC);
*/