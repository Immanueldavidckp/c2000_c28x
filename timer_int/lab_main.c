#include "driverlib.h"
#include "device.h"
#include "board.h"

#define LED_PIN   23U

volatile uint32_t timerTickCount = 0;


__interrupt void INT_TIM0_ISR(void)
{
    timerTickCount++;
    GPIO_togglePin(LED_PIN);
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP1);
}

void main(void)
{
    Device_init();
    Device_initGPIO();

    Interrupt_initModule();
    Interrupt_initVectorTable();

    Board_init();                      

    GPIO_setPadConfig(LED_PIN, GPIO_PIN_TYPE_STD);
    GPIO_setDirectionMode(LED_PIN, GPIO_DIR_MODE_OUT);
    GPIO_writePin(LED_PIN, 1);          

    GPIO_setPadConfig(34U, GPIO_PIN_TYPE_STD);
    GPIO_setDirectionMode(34U, GPIO_DIR_MODE_OUT);
    GPIO_writePin(34U, 1); 

    Interrupt_enable(INT_TIM0);      

    CPUTimer_startTimer(TIM0_BASE);

    EINT;
    ERTM;

    while (1)
    {
        if((CPUTimer_getTimerOverflowStatus(TIM0_BASE)))
        {
        GPIO_togglePin(34U);
        DEVICE_DELAY_US(1000000);
        }
        
    }
}