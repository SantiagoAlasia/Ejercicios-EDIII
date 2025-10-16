/*
 * @brief Enciende un LED conectado al p0.22 cuando el tiempo entre dos eventos
 * consecutivos asosciados a un flanco ascendente en el p1.26 debido a la interripcion
 * del TMR0, sea mayor o igual a 1 segundo.
*/

# include "LPC17xx.h"
# include "lpc17xx_gpio.h"
# include "lpc17xx_pinsel.h"
# include "lpc17xx_timer.h"

volatile uint32_t lastCapValue = 0;
volatile uint32_t currCapValue = 0;
volatile uint8_t capFlag = 0;

void cfgGPIO(void);
void cfgTimer(void);

int main(void){
    cfgGPIO();
    cfgTimer();

    while(1){}

    return 0;
}

/*Definicion de Funciones*/
void cfgGPIO(void){
    PINSEL_CFG_Type cfgPinLed; //Configuracion del p0.22
    PINSEL_CFG_Type cfgPinCAP0; //Configuracion del p1.26 (pin que usa el CAP0)

    cfgPinLed.Portnum = PINSEL_PORT_0;
    cfgPinLed.Pinnum = PINSEL_PIN_22;
    cfgPinLed.Funcnum = PINSEL_FUNC_0; //Funcionalidad GPIO
    cfgPinLed.Pinmode = PINSEL_PINMODE_TRISTATE; //Pin flotante (Ni pull up, ni pull down)
    cfgPinLed.OpenDrain = PINSEL_PINMODE_NORMAL;

    GPIO_SetDir(PORT_0, PIN_22, OUTPUT); //p0.22 Output

    cfgPinCAP0.Portnum = PINSEL_PORT_1;
    cfgPinCAP0.Pinnum = PINSEL_PIN_26;
    cfgPinCAP0.Funcnum = PINSEL_FUNC_3; //Funcionalidad MAT
    cfgPinCAP0.OpenDrain = PINSEL_PINMODE_NORMAL;

    PINSEL_configPin(&cfgPinLed);
    PINSEL_configPin(&cfgPinCAP0);

    return;
}

void cfgTimer(void){
    TIM_COUNTERCFG_Type cfgTimerMode; //Configuracion de Timer en modo temporizador
    TIM_CAPTURECFG_Type cfgTimerCapture; //Configuracion de los eventos de Capture 0

    cfgTimerMode.CountInputSelect = TIM_COUNTER_INCAP0; //Pin de entrada CAP0.0 para TMR0

    cfgTimerCapture.CaptureChannel = 0;
    cfgTimerCapture.RisingEdge = ENABLE;
    cfgTimerCapture.FallingEdge = DISABLE;
    cfgTimerCapture.IntOnCapture = ENABLE; //Habilitacion de interrupcion por el evento de capture

    TIM_Init(LPC_TIM0, TIM_COUNTER_RISING_MODE, &cfgTimerMode); //Configuracion de la modalidad del TMR0
    TIM_ConfigCapture(LPC_TIM0, &confTimerCapture); //Configuracion del TMR0 como temporizador
    TIM_Cmd(LPC_TIM0, ENABLE); //Habilitacion de cuenta de TMR0

    NVIC_EnableIRQ(TIMER0_IRQn); //Habilitacion de interrupcion por TMR0

    return;
}

void TIMER0_IRQHandler(void){
    if(TIM_GetIntCaptureStatus(LPC_TIM0, TIM_CR0_INT)){ //Consulta por interrupcion por CR0
        currCapValue = TIM_GetCaptureValue(LPC_TIM0, TIM_COUNTER_INCAP0); //Valor del registro de captura CAP0

        if(capFlag == 0){
            //Guarda la primer captura y se va
            lastCapValue = currCapValue;
            capFlag = 1;
        }else{
            //Calcula la diferencia de ticks entre dos capturas
            uint32_t diffTicks = (currCapValue - lastCapValue); //Ticks transcurridos entre dos eventos

            float tickTime = 1.0f / 25000000.0f //Considerando que CCLK = 100MHz => PCLK = CCLK/4 = 25MHz => Cada tick es de 40ns
            float diffTime = diffTicks * tickTime;

            if(diffTime >= 1.0f){
               //Si paso 1 segundo o mas...
               GPIO_SetValue(PORT_0, PIN_22);
            }

            lastCapValue = currCapValue;
        }
    }
    TIM_ClearIntPending(LPC_TIM0, TIM_CR0_INT); //Limpio la bandera de la interrupcion por TMR0
    return;
}