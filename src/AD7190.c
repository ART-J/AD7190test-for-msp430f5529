#include "AD7190.h"
#include "math.h"
#include "usci_b1_spi.h"
#include <msp430.h>

#define V_Ref 3     //�ο���ѹ

#define AD7190_Write                0
#define AD7190_Read                 1

//�Ĵ���ѡ��
#define ComState_register   0//д�����ڼ�Ϊͨ�żĴ���//�������ڼ�Ϊ״̬�Ĵ���
#define Mode_register       1//ģʽ�Ĵ���
#define Config_register     2//���üĴ���
#define Data_register       3//���ݼĴ���/���ݼĴ�����״̬��Ϣ
#define ID_register         4//ID�Ĵ���
#define GPOCON_register     5//GPOCON�Ĵ���
#define Disorders_register  6//ʧ���Ĵ���
#define FullScale_register  7//�����̼Ĵ���

//ģʽ�Ĵ���
#define MODE_ADC_OneByOne       0//����ģʽ������ת��
#define MODE_ADC_OnlyOne        1//����ģʽ������ת��
#define MODE_ADC_Free           2//����ģʽ������ģʽ
#define MODE_ADC_SavePower      3//����ģʽ��ʡ��ģʽ
#define MODE_ADC_AdjustZero     4//����ģʽ���ڲ���ƽУ׼
#define MODE_ADC_AdjustFull     5//����ģʽ������������У׼
#define MODE_ADC_SysAdjustZero  6//����ģʽ��ϵͳ���ƽУ׼
#define MODE_ADC_SysAdjustFull  7//����ģʽ��ϵͳ������У׼

#define MODE_ADC_DAT_STA        1

#define MODE_MCLK_OUTosc        0//�ⲿ����
#define MODE_MCLK_OUTclo        1//�ⲿʱ��
#define MODE_MCLK_IN            2//4.92Mhz�ڲ�ʱ�ӣ�MCLK2����Ϊ��̬
#define MODE_MCLK_INcloOut      3//4.92Mhz�ڲ�ʱ�ӣ��ڲ�ʱ�ӿ��Դ�MCLK2���
#define MODE_SINC3              1//���㣬sinc4�˲�������λ��sinc3�˲�����
#define MODE_ENPAR              1//ʹ����żУ��λ
#define MODE_Single             1//MR11��������ת��ʹ��
#define MODE_REJ60              1//MR10��ʹ��һ��60Hz���岨
//#define MODE_Filter_Speed     0
//���üĴ���
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

                                    //ADC���뷶Χ��+-��
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
u32 EMPTY12:1;      //��
u32 ENPAR:1;
u32 EMPTY14:1;      //��
u32 SINC3_EN:1;
u32 EMPTY17_16:2;   //��
u32 ADC_SCLK:2;
u32 Return_state:1;
u32 ADC_Mode:3;
}AD7190_MODE_SET;//ģʽ�Ĵ���24λ

typedef struct {
u32 Config_ADC_Gain:3;
u32 Config_UB:1;
u32 Config_BUF:1;
u32 EMPTY5:1;           //��
u32 Config_REFDET:1;
u32 Config_Burn:1;
u32 Config_Channel:8;
u32 EMPTY19_16:4;          //��
u32 Config_REFSEL:1;
u32 EMPTY22:2;          //��
u32 Config_Chop:1;    //ն��ʹ��
}AD7190_Config_SET;//���üĴ���24λ

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
//�ȴ����ݾ���
/*------------------------------------------------------------------*/
void WaitDataRDY(void)
{
    u8 buf;
    while(1)//����δ������ѭ���ȴ�
    {                       //01000000
        AD7190_Connect_Set(0,AD7190_Read,ComState_register);//��״̬�Ĵ���
        ReadFromAD7190(1,&buf);
        if((buf&0x40))//�ж�ADC����λ�Ƿ���λ
        {//01000000
            delay_ms(10);
            return 0;
        }else if(!(buf&0x80))//��ȡADC���ݼĴ���֮�󣬻�������ת���������֮ǰ��һ��ʱ���ڣ����Զ���1
        {//10000000
            return 1;//���ݾ���
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
    CLR_CS();//Ƭѡ����
    delay_us(1);
    usci_b1_spi_transmit_frame(buf_t,buf_r ,count);
//    SET_CS();//Ƭѡ����
}
//Function that reads from the AD7190 via the SPI port.
//--------------------------------------------------------------------------------
void ReadFromAD7190(unsigned char count, unsigned char *buf_r)
{
    unsigned char buf_t[5]={0x00,0x00,0x00,0x00,0x00};
    CLR_CS();//Ƭѡ����
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

/* Write_EN д��ʹ��λ����ͨ�żĴ���ִ��д���������뽫0д���λ
 * WR 0��ָ���Ĵ���д������1��ָ���Ĵ���������
 * dat���Ĵ���ѡ�񣨼Ĵ�����ַ��
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

    WriteToAD7190(1,&Dat);//00xxx000  xxx<-Dat(д���ַΪDat�ļĴ���)
    WriteToAD7190(3,Write_dat);//����ģʽ�Ĵ��� 3*8λ
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

    WriteToAD7190(1,&Dat);//00xxx000  xxx<-Dat(д���ַΪDat�ļĴ���)
    WriteToAD7190(3,Write_dat);//�������üĴ��� 3*8λ
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
 * ͨ��ѡ��<-����
 */
//  Config.Config_Channel       =       Config_Ch2_temp;
//  Config.Config_Channel       =       Config_Ch1_A3A4|Config_Ch0_A1A2;//Config_Ch4_A1AC
//  Config.Config_Channel       =       Config_Ch4_A1AC|Config_Ch5_A2AC|Config_Ch6_A3AC|Config_Ch7_A4AC;
//  Config.Config_Channel       =       Config_Ch0_A1A2;//ͨ��AIN1+~AIN2
    Config.Config_Channel       =       Config_Ch4_A1AC;
 /*
 * ���üĴ���<-����
 */
    Config.Config_ADC_Gain      =       Config_ADC_Gain_1;//ADC����
    ADC_Gain=1;
    Config.Config_Chop          =       0;//ն��ʹ�ܣ����㣬���ã�
    Config.Config_REFSEL        =       0;//��׼ѡ����REFIN1(+)��REFIN1(-)֮�䣩
    Config.Config_Burn          =       0;//500nA�������������ã�
    Config.Config_REFDET        =       1;//��ѹ��׼��⹦�ܣ�ʹ�ܣ�
    Config.Config_BUF           =       0;//ģ������˵Ļ����������ã��������Խ��迹Դ����ǰ�ˣ�
    Config.Config_UB            =       1;//����ѡ��λ���������Թ�����
    Config.EMPTY19_16         =   0x00;
/*
 * ģʽ�Ĵ���<-����
 */
    Mode.ADC_Mode           =   MODE_ADC_AdjustZero;
    Mode.Return_state       =   0;
    Mode.ADC_SCLK           =   MODE_MCLK_IN;
    Mode.SINC3_EN           =   0;
    Mode.ENPAR              =   0;
    Mode.Single_EN          =   0;
    Mode.REJ60_EN           =   0;
    Mode.Filter             =   1;


//    AD7190_Connect_Set(0,AD7190_Write,0x5C);//дͨ�żĴ�����ʹ��������ȡ��
//    AD7190_Connect_Set(0,AD7190_Write,0x58);//дͨ�żĴ���������������ȡ��

    AD7190_Config_Set(&Config); //�������üĴ���
    WaitDataRDY();

    AD7190_Mode_Set(&Mode);     //����ģʽ�Ĵ�����                                  �ڲ���ƽУ׼
    WaitDataRDY();

    Mode.ADC_Mode           =   MODE_ADC_AdjustFull;
    AD7190_Mode_Set(&Mode);     //����ģʽ�Ĵ���                                      ����������У׼
    WaitDataRDY();

    Mode.Return_state       =   MODE_ADC_DAT_STA;
    Mode.ADC_Mode           =   MODE_ADC_OneByOne;//    ����ת��                            fmod=MCLK��4.92Mhz��/16
    Mode.Filter             =   10;//�˲������ѡ������ѡ��λ       * ���������������=��fmod/64��/FS     ����ն��
    AD7190_Mode_Set(&Mode);        //����ģʽ�Ĵ���                            * ���������������=��fmod/64��/��FS*N��  ʹ��ն��
//
//
//    AD7190_Connect_Set(0,AD7190_Read,Disorders_register);
//    ReadFromAD7190(3,&buf[0]);      //��ȡʧ���Ĵ���
//    DisordersData = (((u32)buf[0])<<16)|(((u32)buf[1])<<8)|(((u32)buf[2]));//ƴ������
//    DisordersTemp = (DisordersData&0X007FFFFF)^0X007FFFFF;
//    DisordersTemp = (DisordersData&0XFF7FFFFF)^0XFF800000+1;
//    DisordersData = (DisordersData>=0X00800000)?(-(DisordersTemp+1)):(DisordersData);//��ֵ����
}
void ad7190_Init(void)
{
    u8 buf[3];
//    IO�ڳ�ʼ��
    P3DIR|=BIT5;

/*40��ʱ�Ӹߵ�ƽ��λ-----------------------------------------------*/
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

/*-��ȡ������ADC�ĵ�ѹֵ��ͨ��------------------------------------------------*/
Vo_cl ADC_Votage(void)
{
    Vo_cl VCtemp;
    u8 buf[3];
    u32 tempn;
    WaitDataRDY();

    buf[0] = 0x58;//01011000
    WriteToAD7190(1,buf);       //write communication register 0x58 to control the progress to read data register

    tempn=GET_AD7190_C();
    VCtemp.Voltage=(tempn>>8)*V_Ref/(float)(16777216*ADC_Gain);//�����ѹֵ
    VCtemp.Channel=(tempn)&0x00000007;//��ȡͨ��ֵ

//  tempn=tempn>>8;     //���˲����24bit����ת��Ϊ16bit��

    return VCtemp;
}
/*-��ȡADC����ֵ------------------------------------------------*/
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
