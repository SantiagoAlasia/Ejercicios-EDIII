/*
 * @brief Enciende y apaga un led conectado en p0.22 de forma intermitente
 * cada 1s debido a la interrupción de TMR.
 */

# include "LPC17xx.h"
# include "lpc17xx_gpio.h"
# include "lpc17xx_pinsel.h"
# include "lpc17xx_timer.h"

void cfgGPIO(void);
void cfgTimer(void);

int main (void){
    cfgGPIO();
    cfgTimer();

    while(1){}

    return 0;
}

/*Definicion de Funciones*/
void cfgGPIO(void){
    PINSEL_CFG_Type cfgPinLed; //Configuracion del p0.22
    PINSEL_CFG_Type cfgPinMAT0; //Configuracion del p1.28 (pin que usa el MAT0)

    cfgPinLed.Portnum = PINSEL_PORT_0;
    cfgPinLed.Pinnum = PINSEL_PIN_22;
    cfgPinLed.Funcnum = PINSEL_FUNC_0; //Funcionalidad GPIO
    cfgPinLed.Pinmode = PINSEL_PINMODE_TRISTATE; //Pin flotante (Ni pull up, ni pull down)
    cfgPinLed.OpenDrain = PINSEL_PINMODE_NORMAL;

    GPIO_SetDir(PORT_0, PIN_22, OUTPUT); //p0.22 Output

    cfgPinMAT0.Portnum = PINSEL_PORT_1;
    cfgPinMAT0.Pinnum = PINSEL_PIN_28;
    cfgPinMAT0.Funcnum = PINSEL_FUNC_3; //Funcionalidad MAT
    cfgPinMAT0.OpenDrain = PINSEL_PINMODE_NORMAL;

    PINSEL_configPin(&cfgPinLed);
    PINSEL_configPin(&cfgPinMAT0);

    return;
}

void cfgTimer(void){
    TIM_TIMERCFG_Type cfgTimerMode; //Configuracion de Timer en modo temporizador
    TIM_MATCHCFG_Type cfgTimerMatch; //Configuracion del tiempo de desborde de MATCH

    cfgTimerMode.PrescaleOption = TIM_PRESCALE_USVAL; //Base de timepo del prescaler a partir de valor en us
    cfgTimerMode.PrescaleValue = 1000; //Base de tiempo de desborde de prescaler en us (Time_Overflow_PC)

    cfgTimerMatch.MatchChannel = 0; //Habilitacion de canal de MATCH0
    cfgTimerMatch.MatchValue = 999; //Valor maximo para el TC (MR)
    cfgTimerMatch.IntOnMatch = ENABLE; //Habilitacion de Interrupcion por MATCH
    cfgTimerMatch.ResetOnMatch = ENABLE; //Habilitacion de Reset o MATCH
    cfgTimerMatch.StopOnMatch = DISABLE; //Deshabbilitacion de stop por MATCH
    cfgTimerMatch.ExtMatchOutputType = TIM_EXTMATCH_NOTHING; //Modalidad del pin de MATCH (Aunque no lo usemos hay q configurarlo arriba en el cfgGPIO)

    TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &cfgTimerMode); //Configuracion de la modalidad del TMR0
    TIM_ConfigMatch(LPC_TIM0, &confTimerMatch); //Configuracion del TMR0 como temporizador
    TIM_Cmd(LPC_TIM0, ENABLE); //Habilitacion de cuenta de TMR0

    NVIC_EnableIRQ(TIMER0_IRQn); //Habilitacion de interrupcion por TMR0

    return;
}

void TIMER0_IRQHandler(void){
    if(TIM_GetIntStatus(LPC_TIM0, TIM_MR0_INT)){     //Consulta por la interrupcion por MR0 (Leo la bandera)
        static uint8_t i = 0;
        if(i == 0){
            GPIO_SetValue(PORT_0, PIN_22);
            i == 1;
        }else if(i == 1){
            GPIO_ClearValue(PORT_0, PIN_22);}
             i = 0;
        }
    }

    TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT); //Limpia la bandera de la interrupcion por TMR0

    return;
}