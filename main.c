/**
 * main.c
 */
#include <msp430.h>
//#include <UCS_INIT.h>
#include "AD7190.h"
#include "usci_b1_spi.h"
#include "ucs.h"

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer
//    UCS_init();
    ucs_init();

    usci_b1_spi_init();//”≤º˛SPI≥ı ºªØ

    ad7190_Init();
//  _EINT();

    Vo_cl VC;
    u8 buf1[1] = {0x50};//01010000,∂¡≈‰÷√ºƒ¥Ê∆˜
    u8 buf[3];
    while(1){
//
//        buf[0]=0x00;
//        buf[1]=0x00;
//        buf[2]=0x00;
//        WaitDataRDY();
//
//        WriteToAD7190(1,buf1);
//
//        ReadFromAD7190(4,buf);

      VC=ADC_Votage();


    }
}
