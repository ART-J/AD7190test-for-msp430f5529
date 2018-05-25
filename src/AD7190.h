/*
 * AD7190.h
 *
 *  Created on: 2018年5月15日
 *      Author: mudi6
 */

#ifndef _AD7190_H_
#define _AD7190_H_


#define CPU_F ((double)25000000)
#define delay_us(x) __delay_cycles((long)(CPU_F*(double)x/1000000.0))
#define delay_ms(x) __delay_cycles((long)(CPU_F*(double)x/1000.0))


#define u32 unsigned long
#define u8 unsigned char

typedef struct {
float Voltage;
u8 Channel;
}Vo_cl;//电压，通道

void WaitDataRDY(void);
void WriteToAD7190(unsigned char count, unsigned char *buf_t);
void ReadFromAD7190(unsigned char count, unsigned char *buf_r);

void AD7190_Connect_Set(u8 Write_EN,u8 WR,u8 dat);

unsigned long GET_AD7190(void);
unsigned long GET_AD7190_C(void);

void ad7190_Init(void);

Vo_cl ADC_Votage(void);
u32 ADC_Num(void);

extern u8 ADC_Channel;

#endif /* SRC_AD7190_H_ */

////extern long int DisordersData;
////////0
////////1           PCout(11)       SPI_SCK
////////2           PCout(10)       SET_SDO         DIN
////////3           PCout(9)        SPI_DOUT        DOUT
////////4           PCout(12)       SET_CS
////#define SPI_DOUT        PCin(9)
////#define SPI_SCK         PCout(11)
////#define GP0DAT          PCin(9)
////#define SET_CS()        PCout(12)=1 //P4.3->/CS
////#define CLR_CS()        PCout(12)=0
////
////#define SET_SCL()       PCout(11)=1 //P4.4->SCLK
////#define CLR_SCL()       PCout(11)=0
////
////#define SET_SDO()       PCout(10)=1 //P4.5->DIN
////#define CLR_SDO()       PCout(10)=0
///*
//COMMUNICATIONS REGISTER :
//CR7     CR6     CR5         CR4     CR3     CR2         CR1   CR0
//WEN(0)   R/W(0)   RS2(0)   RS1(0)   RS0(0)   CREAD(0)   0(0)   0(0)
//
//
//RS2   RS1   RS0   Register                              Register
//0       0   0       Communications register during a write operation    8 bits
//0       0   0       Status register during a read operation         8 bits
//0       0   1       Mode register                       24 bits
//0       1   0       Configuration register                  24 bits
//0       1   1       Data register/data register plus status information     24 bits/32
//1       0   0       ID register                         8 bits
//1       0   1       GPOCON register                     8 bits
//1       1   0       Offset register                     24 bits
//1       1   1       Full-scale register                     24 bits
//
//STATUS REGISTER :
//
//SR7         SR6     SR5         SR4     SR3     SR2     SR1         SR0
//RDY(1)   ERR(0)   NOREF(0)   Parity(0)   0(0)  CHD2(0)  CHD1(0)   CHD0(0)
//*/
///*----------------------------------------------------------------------
//  File Name          : AD7190_IO.h
//  Author             : Neil Zhao - CAST
//  Version            : V1.0
//  Date               : 20/2/2009
//  Description        : AD7190 Driver Declaration
//
//  The AD7190 is a low noise, complete analog front end for high
//  precision measurement applications.
//
//  China Applications Support Team.
//  china.support@analog.com
//
//----------------------------------------------------------------------
//
//  The present firmware which is for guidance only aims at providing
//  customers with coding information regarding their products in order
//  for them to save time.  As a result, Analog Devices shall not be
//  held liable for any direct, indirect or consequential damages with
//  respect to any claims arising from the content of such firmware and/or
//  the use made by customers of the coding information contained herein
//  in connection with their products.
//
//----------------------------------------------------------------------*/
//
////---------------------------------
////void WriteToAD7190(unsigned char count,unsigned char *buf);
////---------------------------------
////Function that writes to the AD7190 via the SPI port.
////--------------------------------------------------------------------------------
////void WaitDataRDY(void);
