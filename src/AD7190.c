#include "AD7190.h"
#include "math.h"
#include "usci_b1_spi.h"
#include <msp430.h>

#define V_Ref 3     //参考电压

#define AD7190_Write                0
#define AD7190_Read                 1

//寄存器选择
#define ComState_register   0//写操作期间为通信寄存器//读操作期间为状态寄存器
#define Mode_register       1//模式寄存器
#define Config_register     2//配置寄存器
#define Data_register       3//数据寄存器/数据寄存器加状态信息
#define ID_register         4//ID寄存器
#define GPOCON_register     5//GPOCON寄存器
#define Disorders_register  6//失调寄存器
#define FullScale_register  7//满量程寄存器

//模式寄存器
#define MODE_ADC_OneByOne       0//工作模式，连续转换
#define MODE_ADC_OnlyOne        1//工作模式，单次转换
#define MODE_ADC_Free           2//工作模式，空闲模式
#define MODE_ADC_SavePower      3//工作模式，省电模式
#define MODE_ADC_AdjustZero     4//工作模式，内部电平校准
#define MODE_ADC_AdjustFull     5//工作模式，内置满量程校准
#define MODE_ADC_SysAdjustZero  6//工作模式，系统零电平校准
#define MODE_ADC_SysAdjustFull  7//工作模式，系统满量程校准

#define MODE_ADC_DAT_STA        1

#define MODE_MCLK_OUTosc        0//外部晶振
#define MODE_MCLK_OUTclo        1//外部时钟
#define MODE_MCLK_IN            2//4.92Mhz内部时钟，MCLK2引脚为三态
#define MODE_MCLK_INcloOut      3//4.92Mhz内部时钟，内部时钟可以从MCLK2获得
#define MODE_SINC3              1//清零，sinc4滤波器；置位，sinc3滤波器。
#define MODE_ENPAR              1//使能奇偶校验位
#define MODE_Single             1//MR11，单周期转换使能
#define MODE_REJ60              1//MR10，使能一个60Hz陷阱波
//#define MODE_Filter_Speed     0
//配置寄存器
#define Config_Chop_EN          1
#define Config_REFSEL_IO        1
#define Config_Burn_EN          1
#define Config_REFDET_EN        1
#define Config_BUF_EN           1
#define Config_UB_EN            1

#define Config_Ch0_A1A2         0x01
#define Config_Ch1_A3A4         0x02
#define Config_Ch2_temp         0x04
#define Config_Ch3_A2A2         0x08
#define Config_Ch4_A1AC         0x10
#define Config_Ch5_A2AC         0x20
#define Config_Ch6_A3AC         0x40
#define Config_Ch7_A4AC         0x80

                                    //ADC输入范围（+-）
#define Config_ADC_Gain_1       0   //5V
#define Config_ADC_Gain_8       3   //625mv
#define Config_ADC_Gain_16      4   //312.5mv
#define Config_ADC_Gain_32      5   //156.2
#define Config_ADC_Gain_64      6   //78.125mv
#define Config_ADC_Gain_128     7   //39.06mv

typedef struct {
u32 Filter:10;
u32 REJ60_EN:1;
u32 Single_EN:1;
u32 EMPTY12:1;      //空
u32 ENPAR:1;
u32 EMPTY14:1;      //空
u32 SINC3_EN:1;
u32 EMPTY17_16:2;   //空
u32 ADC_SCLK:2;
u32 Return_state:1;
u32 ADC_Mode:3;
}AD7190_MODE_SET;//模式寄存器24位

typedef struct {
u32 Config_ADC_Gain:3;
u32 Config_UB:1;
u32 Config_BUF:1;
u32 EMPTY5:1;           //空
u32 Config_REFDET:1;
u32 Config_Burn:1;
u32 Config_Channel:8;
u32 EMPTY19_16:4;          //空
u32 Config_REFSEL:1;
u32 EMPTY22:2;          //空
u32 Config_Chop:1;    //斩波使能
}AD7190_Config_SET;//配置寄存器24位

u8 ADC_Channel,text;
u8 shuju;
u32 ADC_Gain;

inline void CLR_CS(void)
{
    P3OUT&=~BIT5;
}
inline void SET_CS(void)
{
    P3OUT|=BIT5;
}


/*------------------------------------------------------------------*/
//等待数据就绪
/*------------------------------------------------------------------*/
void WaitDataRDY(void)
{
    u8 buf;
    while(1)//数据未就绪，循环等待
    {                       //01000000
        AD7190_Connect_Set(0,AD7190_Read,ComState_register);//读状态寄存器
        ReadFromAD7190(1,&buf);
        if((buf&0x40))//判断ADC错误位是否置位
        {//01000000
            delay_ms(10);
            return 0;
        }else if(!(buf&0x80))//读取ADC数据寄存器之后，或者在新转换结果更新之前的一定时间内，会自动置1
        {//10000000
            return 1;//数据就绪
        }
        delay_ms(3);
    }
//  while(SPI_DOUT); //here miso(Dout) also work as !DRD
}


/*--------------------------------------------------------------------------------*/
//Function that writes to the AD7190 via the SPI port.
//--------------------------------------------------------------------------------
void WriteToAD7190(unsigned char count, unsigned char *buf_t)
{
    unsigned char buf_r[5]={0x00,0x00,0x00,0x00,0x00};
    CLR_CS();//片选拉低
    delay_us(1);
    usci_b1_spi_transmit_frame(buf_t,buf_r ,count);
//    SET_CS();//片选拉高
}
//Function that reads from the AD7190 via the SPI port.
//--------------------------------------------------------------------------------
void ReadFromAD7190(unsigned char count, unsigned char *buf_r)
{
    unsigned char buf_t[5]={0x00,0x00,0x00,0x00,0x00};
    CLR_CS();//片选拉低
    delay_us(1);
    usci_b1_spi_transmit_frame(buf_t,buf_r,count);
//    SET_CS();
}
/*----------------------------------------------------------------------------------*/


unsigned long GET_AD7190(void)
{
    u32 DAC_Dat=0;
    u8 buf[3];

    ReadFromAD7190(3,buf);

        AD7190_Connect_Set(0,AD7190_Read,ComState_register);
        ReadFromAD7190(1,&text);
        ADC_Channel=text&0x07;

    DAC_Dat=buf[2]*256*256+buf[1]*256+buf[0];

    return DAC_Dat;
}
unsigned long GET_AD7190_C(void)
{
    u32 DAC_Dat=0;
    u8 buf[4];

    ReadFromAD7190(4,buf);
    DAC_Dat=(u32)buf[0]*256*256*256+(u32)buf[1]*256*256+(u32)buf[2]*256+(u32)buf[3];

    return DAC_Dat;
}

/* Write_EN 写入使能位，对通信寄存器执行写操作，必须将0写入此位
 * WR 0，指定寄存器写操作；1，指定寄存器读操作
 * dat，寄存器选择（寄存器地址）
 */
void AD7190_Connect_Set(u8 Write_EN,u8 WR,u8 dat)
{
    u8 buf=dat;

    if(Write_EN)buf=1<<7;
    if(WR)  buf=1<<6;
    WriteToAD7190(1,&buf);
}

/*
 *
 */
void AD7190_Mode_Set(AD7190_MODE_SET *Mode)
{
    u8 Write_dat[3],Dat;
    Dat=Mode_register<<3;

    Write_dat[0]=((u8*)Mode)[2];
    Write_dat[1]=((u8*)Mode)[1];
    Write_dat[2]=((u8*)Mode)[0];

    WriteToAD7190(1,&Dat);//00xxx000  xxx<-Dat(写入地址为Dat的寄存器)
    WriteToAD7190(3,Write_dat);//发送模式寄存器 3*8位
}

/*
 *
 */
void AD7190_Config_Set(AD7190_Config_SET *Mode)
{
    u8 Write_dat[3],Dat;
    Dat=Config_register<<3;

    Write_dat[0]=((u8*)Mode)[2];
    Write_dat[1]=((u8*)Mode)[1];
    Write_dat[2]=((u8*)Mode)[0];

    WriteToAD7190(1,&Dat);//00xxx000  xxx<-Dat(写入地址为Dat的寄存器)
    WriteToAD7190(3,Write_dat);//发送配置寄存器 3*8位
}


long int DisordersData = 0;
long int DisordersTemp = 0;

/*
 *
 */
void AD7190_Init(void)
{
//    u8 buf[5];
    AD7190_MODE_SET     Mode;
    AD7190_Config_SET   Config;

/*
 * 通道选择<-配置
 */
//  Config.Config_Channel       =       Config_Ch2_temp;
//  Config.Config_Channel       =       Config_Ch1_A3A4|Config_Ch0_A1A2;//Config_Ch4_A1AC
//  Config.Config_Channel       =       Config_Ch4_A1AC|Config_Ch5_A2AC|Config_Ch6_A3AC|Config_Ch7_A4AC;
//  Config.Config_Channel       =       Config_Ch0_A1A2;//通道AIN1+~AIN2
    Config.Config_Channel       =       Config_Ch4_A1AC;
 /*
 * 配置寄存器<-配置
 */
    Config.Config_ADC_Gain      =       Config_ADC_Gain_1;//ADC增益
    ADC_Gain=1;
    Config.Config_Chop          =       0;//斩波使能（清零，禁用）
    Config.Config_REFSEL        =       0;//基准选择（在REFIN1(+)和REFIN1(-)之间）
    Config.Config_Burn          =       0;//500nA激励电流（禁用）
    Config.Config_REFDET        =       1;//电压基准检测功能（使能）
    Config.Config_BUF           =       0;//模拟输入端的缓冲器（禁用），（可以将阻抗源置于前端）
    Config.Config_UB            =       1;//极性选择位，（单极性工作）
    Config.EMPTY19_16         =   0x00;
/*
 * 模式寄存器<-配置
 */
    Mode.ADC_Mode           =   MODE_ADC_AdjustZero;
    Mode.Return_state       =   0;
    Mode.ADC_SCLK           =   MODE_MCLK_IN;
    Mode.SINC3_EN           =   0;
    Mode.ENPAR              =   0;
    Mode.Single_EN          =   0;
    Mode.REJ60_EN           =   0;
    Mode.Filter             =   1;


//    AD7190_Connect_Set(0,AD7190_Write,0x5C);//写通信寄存器（使能连续读取）
//    AD7190_Connect_Set(0,AD7190_Write,0x58);//写通信寄存器（禁用连续读取）

    AD7190_Config_Set(&Config); //发送配置寄存器
    WaitDataRDY();

    AD7190_Mode_Set(&Mode);     //发送模式寄存器，                                  内部电平校准
    WaitDataRDY();

    Mode.ADC_Mode           =   MODE_ADC_AdjustFull;
    AD7190_Mode_Set(&Mode);     //发送模式寄存器                                      内置满量程校准
    WaitDataRDY();

    Mode.Return_state       =   MODE_ADC_DAT_STA;
    Mode.ADC_Mode           =   MODE_ADC_OneByOne;//    连续转换                            fmod=MCLK（4.92Mhz）/16
    Mode.Filter             =   10;//滤波器输出选择速率选择位       * 输出数据数据速率=（fmod/64）/FS     禁用斩波
    AD7190_Mode_Set(&Mode);        //发送模式寄存器                            * 输出数据数据速率=（fmod/64）/（FS*N）  使能斩波
//
//
//    AD7190_Connect_Set(0,AD7190_Read,Disorders_register);
//    ReadFromAD7190(3,&buf[0]);      //读取失调寄存器
//    DisordersData = (((u32)buf[0])<<16)|(((u32)buf[1])<<8)|(((u32)buf[2]));//拼接数据
//    DisordersTemp = (DisordersData&0X007FFFFF)^0X007FFFFF;
//    DisordersTemp = (DisordersData&0XFF7FFFFF)^0XFF800000+1;
//    DisordersData = (DisordersData>=0X00800000)?(-(DisordersTemp+1)):(DisordersData);//数值计算
}
void ad7190_Init(void)
{
    u8 buf[3];
//    IO口初始化
    P3DIR|=BIT5;

/*40个时钟高电平复位-----------------------------------------------*/
    buf[0] = 0xff;
    WriteToAD7190(1,buf);

    buf[0] = 0xff;
    WriteToAD7190(1,buf);

    buf[0] = 0xff;
    WriteToAD7190(1,buf);

    buf[0] = 0xff;
    WriteToAD7190(1,buf);

    buf[0] = 0xff;
    WriteToAD7190(1,buf);
/*------------------------------------------------*/
    AD7190_Init();
}

/*-获取并计算ADC的电压值与通道------------------------------------------------*/
Vo_cl ADC_Votage(void)
{
    Vo_cl VCtemp;
    u8 buf[3];
    u32 tempn;
    WaitDataRDY();

    buf[0] = 0x58;//01011000
    WriteToAD7190(1,buf);       //write communication register 0x58 to control the progress to read data register

    tempn=GET_AD7190_C();
    VCtemp.Voltage=(tempn>>8)*V_Ref/(float)(16777216*ADC_Gain);//计算电压值
    VCtemp.Channel=(tempn)&0x00000007;//获取通道值

//  tempn=tempn>>8;     //把滤波后的24bit数据转换为16bit。

    return VCtemp;
}
/*-获取ADC的数值------------------------------------------------*/
u32 ADC_Num(void)
{
    u8 buf[3];
    u32 tempn;

    WaitDataRDY();

    buf[0] = 0x58;
    WriteToAD7190(1,buf); //write communication register 0x58 to control the progress to read data register

    tempn=GET_AD7190();

    return tempn;
}
