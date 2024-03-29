/**
 * UART test code:
 * Echo's characters back that are inputted.
 *
 * Author: Emad Khan, ECED4402 TA
 * Summer 2017
 */
#include "Uart.h"


/* Globals */
volatile char UART_data; /* Input data from UART receive */
volatile int UART_busy = FALSE; /* T|F - Data available from UART */



void UART0_Init(void)
{
    volatile int wait;

    /* Initialize UART0 */
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCUART_GPIOA; // Enable Clock Gating for UART0
    SYSCTL_RCGCUART_R |= SYSCTL_RCGCGPIO_UART0; // Enable Clock Gating for PORTA
    wait = 0; // give time for the clocks to activate

    UART0_CTL_R &= ~UART_CTL_UARTEN;        // Disable the UART
    wait = 0;   // wait required before accessing the UART config regs

    // Setup the BAUD rate
    UART0_IBRD_R = IBRD_115200; // IBRD = int(16,000,000 / (16 * 115,200)) = 8.680555555555556
    UART0_FBRD_R = FBRD_115200; // FBRD = int(.680555555555556 * 64 + 0.5) = 44.05555555555556

    UART0_LCRH_R = (UART_LCRH_WLEN_8); // WLEN: 8, no parity, one stop bit, without FIFOs)

    GPIO_PORTA_AFSEL_R = 0x3;        // Enable Receive and Transmit on PA1-0
    GPIO_PORTA_PCTL_R = (0x01) | ((0x01) << 4); // Enable UART RX/TX pins on PA1-0
    GPIO_PORTA_DEN_R = EN_DIG_PA0 | EN_DIG_PA1;   // Enable Digital I/O on PA1-0

    UART0_CTL_R = UART_CTL_UARTEN;        // Enable the UART
    wait = 0; // wait; give UART time to enable itself.
}

void UART0_IntEnable(unsigned long flags)
{
    /* Set specified bits for interrupt */
    UART0_IM_R |= flags;
}

void UART0_IntDisable(unsigned long flags)
{
    /* Set specified bits for interrupt */
    UART0_IM_R &= ~flags;
}

void InterruptEnable(unsigned long InterruptIndex)
{
    /* Indicate to CPU which device is to interrupt */
    if (InterruptIndex < 32)
        NVIC_EN0_R = 1 << InterruptIndex; // Enable the interrupt in the EN0 Register
    else
        NVIC_EN1_R = 1 << (InterruptIndex - 32); // Enable the interrupt in the EN1 Register
}

void InterruptMasterEnable(void)
{
    /* enable CPU interrupts */
    __asm(" cpsie   i");
}

int data_xmitting(char data)
{
    UART0_IntDisable(UART_INT_TX);
    //disable();

    if (!UART_busy) //holding register empty
    {
        UART0_DR_R = data;
        UART_busy = TRUE;
        UART0_IntEnable(UART_INT_TX);
        return TRUE;
    }
    else
    {
        UART0_IntEnable(UART_INT_TX);
        return FALSE;
    }
}


void UART0_IntHandler(void)
{
    if (UART0_MIS_R & UART_INT_RX)
    {
        /* RECV done - clear interrupt and make char available to application */
        UART0_ICR_R |= UART_INT_RX;
        UART_data = UART0_DR_R;

        enqueue(UART_IN, UART_data);     //push data to the queue
    }

    if (UART0_MIS_R & UART_INT_TX)
    {
        /* XMIT done - clear interrupt */
        UART0_ICR_R |= UART_INT_TX;

        if (! isEmpty(UART_OUT))
        {
            UART0_DR_R = dequeue(UART_OUT);
            UART_busy = TRUE;
        }
        else
        {
            UART_busy = FALSE;
        }
    }
}


