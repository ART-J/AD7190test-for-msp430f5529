/*
 * usci_b1_spi.c
 *
 *  Created on: 2016-10-20
 *      Author: Jack Chen <redchenjs@live.com>
 * 
 * ---------SPI---------
 * PORT     TYPE    PIN
 * MOSI     OUT     P4.1
 * MISO     IN      P4.2
 * SCK      OUT     P4.3
 * ---------------------
 */

#include <msp430.h>

#define SPI_SET_PIN()   {\
                            P4SEL |= BIT1 + BIT2 + BIT3;\
                        }

static unsigned char *spi_tx_buff;
static unsigned char *spi_rx_buff;

static unsigned char spi_tx_num = 0;

unsigned char usci_b1_spi_transmit_frame(unsigned char *tx_buff, unsigned char *rx_buff, unsigned char num)
{
    if (UCB1STAT & UCBUSY) return 0;
    __disable_interrupt();
    UCB1IE |= (UCTXIE + UCRXIE);
    UCB1IFG &=~(UCTXIFG + UCRXIFG);
    __enable_interrupt();
    spi_tx_buff = tx_buff;
    spi_rx_buff = rx_buff;
    spi_tx_num  = num;
    UCB1TXBUF = *spi_tx_buff++;
    while (UCB1STAT & UCBUSY);
    return 1;
}

inline void usci_b1_spi_tx_isr_handle(void)
{
    spi_tx_num--;
    if (spi_tx_num) {
        UCB1TXBUF = *spi_tx_buff++;
    } else {
        UCB1IFG &=~UCTXIFG;
        UCB1IE &=~(UCTXIE + UCRXIE);
    }
}

inline void usci_b1_spi_rx_isr_handle(void)
{
    *spi_rx_buff++ = UCB1RXBUF;
    UCB1IFG &=~UCRXIFG;
}

void usci_b1_spi_init(void)
{
    SPI_SET_PIN();

    UCB1CTL1 |= UCSWRST;
    UCB1CTL0 |= UCMST + UCMODE_0 + UCSYNC + UCMSB +UCCKPL; //UCCKPH+
    UCB1CTL1 |= UCSSEL_2;

    UCB1BR0  = 2;
    UCB1BR1  = 0;

    UCB1CTL1 &=~UCSWRST;
    UCB1IFG  &=~(UCTXIFG + UCRXIFG);
    __enable_interrupt();
}

#pragma vector=USCI_B1_VECTOR
__interrupt void USCI_B1_ISR(void)
{
    switch (__even_in_range(UCB1IV, 4)) {
    case  0: break;                           // Vector  0: No interrupts
    case  2:                                  // Vector  2: RXIFG
        usci_b1_spi_rx_isr_handle();
        break;
    case  4:                                  // Vector  4: TXIFG
        usci_b1_spi_tx_isr_handle();
        break;
    default: break;
    }
}
