#include "includes.h"
#include "BswDrv_SPI_Fm175xx.h"
#include "BswDrv_SPI.h"

static uint8_t SPIRead(uint8_t addr);
static void SPIWrite(uint8_t addr,uint8_t wrdata);

#define FM1752_POWER_ON()

#define NPD_LOW()           gpio_bit_reset(GPIOB,GPIO_PIN_9)
#define NPD_HIGH()          gpio_bit_set(GPIOB,GPIO_PIN_9)

#define SPI_CS_LOW()        gpio_bit_reset(GPIOA,GPIO_PIN_15)
#define SPI_CS_HIGH()       gpio_bit_set(GPIOA,GPIO_PIN_15)


uint8_t SPIRead(uint8_t addr)
{
	uint8_t  reg_value,send_data;

	SPI_CS_LOW();
	send_data=(addr<<1)|0x80;

	BswDrv_SPI_ReadWriteByte(SPI_FM175x,send_data);
	
	reg_value = BswDrv_SPI_ReadByte(SPI_FM175x);

	SPI_CS_HIGH();

	return(reg_value);
}


void SPIWrite(uint8_t addr,uint8_t wrdata)
{
    uint8_t send_data;
	
    SPI_CS_LOW();
    send_data=(addr<<1)&0x7E;
    BswDrv_SPI_ReadWriteByte(SPI_FM175x,send_data);
    BswDrv_SPI_ReadWriteByte(SPI_FM175x,wrdata);
    
	SPI_CS_HIGH();
}




/*************************************************************/
//��������	    Read_Reg
//���ܣ�	    ���Ĵ�������
//���������	reg_add���Ĵ�����ַ
//����ֵ��	    �Ĵ�����ֵ
/*************************************************************/
uint8_t Read_Reg(uint8_t reg_add) 
{
    uint8_t  reg_value = 0xFF;
    reg_value = SPIRead(reg_add);
	
    return reg_value;
}

uint8_t Write_Reg(uint8_t reg_add, uint8_t reg_value) 
{
    SPIWrite(reg_add, reg_value);
	
    return OK;
}


void Write_FIFO(uint8_t length, uint8_t *fifo_data) 
{
    //SPIWrite_Sequence(length, FIFODataReg, fifo_data);
    unsigned int i = 0;
	
    for (i = 0; i < length; i++) 
	{
        Write_Reg(FIFODataReg, fifo_data[i]);
    }
	
    return;
}

void Read_FIFO(uint8_t length, uint8_t *fifo_data) 
{
    //SPIRead_Sequence(length, FIFODataReg, fifo_data);
    uint8_t i;
	
	for(i = 0; i < length; i++)
	{
		*(fifo_data + i)=Read_Reg(FIFODataReg);
    }
	
    return;
}


uint8_t Clear_FIFO(void) 
{
    Set_BitMask(FIFOLevelReg, 0x80);//���FIFO����
    if (Read_Reg(FIFOLevelReg) == 0)
	{
		return OK;
	}
    else
	{
		return ERROR;
	}
}

uint8_t Clear_BitMask(uint8_t reg_add, uint8_t mask) 
{
    uint8_t  result;
	
    result = Write_Reg(reg_add, Read_Reg(reg_add) & ~mask);  // clear bit mask
    
    return result;
}

/*****************************************************************************
** Function name:       Set_BitMask
** Descriptions:        ��λ�Ĵ�������
** input parameters:    reg_add���Ĵ�����ַ��
                        mask���Ĵ�����λ
** output parameters:   None
** Returned value:	  OK
				        ERROR
** Author:              quqian
*****************************************************************************/
uint8_t Set_BitMask(uint8_t reg_add, uint8_t mask) 
{
    uint8_t   result;
    
    result = Read_Reg(reg_add);
    Write_Reg(reg_add, result | mask);  // set bit mask
    
    return OK;
}

/*****************************************************************************
** Function name:       Set_Rf
** Descriptions:        ������Ƶ���
** input parameters:    mode����Ƶ���ģʽ
            				0���ر����
            				1,����TX1���
            				2,����TX2���
            				3��TX1��TX2�������TX2Ϊ�������
** output parameters:   None
** Returned value:	  OK
				        ERROR
** Author:              quqian
*****************************************************************************/
uint8_t Set_Rf(uint8_t mode) 
{
    uint8_t result;
	
    if ((Read_Reg(TxControlReg) & 0x03) == mode)
	{
		return OK;
	}
    if (mode == 0) 
	{
        result = Clear_BitMask(TxControlReg, 0x03); //�ر�TX1��TX2���
    }
    if (mode == 1) 
	{
        result = Clear_BitMask(TxControlReg, 0x01); //����TX1���
    }
    if (mode == 2) 
	{
        result = Clear_BitMask(TxControlReg, 0x02); //����TX2���
    }
    if (mode == 3) 
	{
        result = Set_BitMask(TxControlReg, 0x03); //��TX1��TX2���
    }
    CARD_DELAY_MS(200);//��TX�������Ҫ��ʱ�ȴ������ز��ź��ȶ�
    
    return result;
}

/*****************************************************************************
** Function name:       Pcd_SetTimer
** Descriptions:        ���ý�����ʱ
** input parameters:    delaytime����ʱʱ�䣨��λΪ���룩
** output parameters:   None
** Returned value:	  OK
** Author:              quqian
*****************************************************************************/
uint8_t Pcd_SetTimer(unsigned long delaytime)//�趨��ʱʱ�䣨ms��
{
    unsigned long  TimeReload = 0;
    unsigned int  Prescaler = 0;

    while (Prescaler < 0xfff) 
	{
        TimeReload = ((delaytime * (long) 13560) - 1) / (Prescaler * 2 + 1);

        if (TimeReload < 0xffff)
		{
			break;
		}
        Prescaler++;
    }
    TimeReload = TimeReload & 0xFFFF;
    Set_BitMask(TModeReg, Prescaler >> 8);
    Write_Reg(TPrescalerReg, Prescaler & 0xFF);
    Write_Reg(TReloadMSBReg, TimeReload >> 8);
    Write_Reg(TReloadLSBReg, TimeReload & 0xFF);
	
    return OK;
}

/*****************************************************************************
** Function name:       Pcd_Comm
** Descriptions:        ������ͨ��
** input parameters:    Command��ͨ�Ų������
            				pInData�������������飻
            				InLenByte���������������ֽڳ��ȣ�
            				pOutData�������������飻
            				pOutLenBit���������ݵ�λ����
** output parameters:   None
** Returned value:	  OK
                        ERROR
** Author:              quqian
*****************************************************************************/
uint8_t Pcd_Comm(uint8_t Command,
                       uint8_t *pInData,
                       uint8_t inLenByte,
                       uint8_t *pOutData,
                       unsigned int *pOutLenBit) 
{
    uint8_t  result;
    uint8_t  rx_temp = 0;//��ʱ�����ֽڳ���
    uint8_t  rx_len = 0;//���������ֽڳ���
    uint8_t  lastBits = 0;//��������λ����
    uint8_t  irq;
    uint8_t InLenByte = inLenByte;
    
    Clear_FIFO();
    
    
    Write_Reg(WaterLevelReg, 0x20);//����FIFOLevel=32�ֽ�
    Write_Reg(ComIrqReg, 0x7F);//���IRQ��־
    if (Command == MFAuthent)
    {
        Write_FIFO(InLenByte, pInData);//������֤��Կ��FIFO���ݼĴ���
        Set_BitMask(BitFramingReg, 0x80);//��������FIFO���ݼĴ����д��������
    }
    Set_BitMask(TModeReg, 0x80);//�Զ�������ʱ��

    Write_Reg(CommandReg, Command);

    while (1)//ѭ���ж��жϱ�ʶ
    {
        irq = Read_Reg(ComIrqReg);//��ѯ�жϱ�־	

        
        if (irq & 0x01)    //TimerIRq  ��ʱ��ʱ���þ�
        {
            result = TIMEOUT_Err ;
            break;
        }
        if (Command == MFAuthent) {
            if (irq & 0x10)    //IdelIRq  command�Ĵ���Ϊ���У�ָ��������
            {
                result = OK;
                break;
            }
        }
        if (Command == Transmit) 
		{
            if ((irq & 0x04) && (InLenByte > 0))    //LoAlertIrq+�����ֽ�������0
            {
                if (InLenByte < 32) 
				{
                    Write_FIFO(InLenByte, pInData);
                    InLenByte = 0;
                } 
				else 
				{
                    Write_FIFO(32, pInData);
                    InLenByte = InLenByte - 32;
                    pInData = pInData + 32;
                }
                Set_BitMask(BitFramingReg, 0x80);    //��������
                Write_Reg(ComIrqReg, 0x04);    //���LoAlertIrq
            }

            if ((irq & 0x40) && (InLenByte == 0))    //TxIRq
            {
                result = OK;
                break;
            }
        }

        if (Command == Transceive) 
		{
            if ((irq & 0x04) && (InLenByte > 0))    //LoAlertIrq + �����ֽ�������0
            {
                if (InLenByte > 32)
                {
                    Write_FIFO(32, pInData);
                    InLenByte = InLenByte - 32;
                    pInData = pInData + 32;
                }
                else 
				{
                    Write_FIFO(InLenByte, pInData);
                    InLenByte = 0;
                }
                Set_BitMask(BitFramingReg, 0x80);//��������
                Write_Reg(ComIrqReg, 0x04);//���LoAlertIrq
            }
            if (irq & 0x08)    //HiAlertIRq
            {
                if ((irq & 0x40) && (InLenByte == 0) &&
                    (Read_Reg(FIFOLevelReg) > 32))//TxIRq	+ �����ͳ���Ϊ0 + FIFO���ȴ���32
                {
                    Read_FIFO(32, pOutData + rx_len); //����FIFO����
                    rx_len = rx_len + 32;
                    Write_Reg(ComIrqReg, 0x08);    //��� HiAlertIRq
                }
            }
            if ((irq & 0x20) && (InLenByte == 0))    //RxIRq=1
            {
                result = OK;
                break;
            }
        }
    }

    {
        if (Command == Transceive)
        {
            rx_temp = Read_Reg(FIFOLevelReg);
            
            lastBits = Read_Reg(ControlReg) & 0x07;

            if ((rx_temp == 0) & (lastBits > 0))   //����յ�����δ��1���ֽڣ������ý��ճ���Ϊ1���ֽڡ�
            {
                rx_temp = 1;
            }

            Read_FIFO(rx_temp, pOutData + rx_len); //����FIFO����

            rx_len = rx_len + rx_temp;//���ճ����ۼ�

            
            if (lastBits > 0)
			{
				*pOutLenBit = (rx_len - 1) * ((unsigned int) 8) + lastBits;
			}
            else
			{
				*pOutLenBit = rx_len * ((unsigned int) 8);
			}
        }
    }
    if (result == OK)
    {
        result = Read_Reg(ErrorReg);
    }

    Set_BitMask(ControlReg, 0x80);     // stop timer now
    Write_Reg(CommandReg, Idle);
    Clear_BitMask(BitFramingReg, 0x80);//�رշ���
    
    return result;
}

/*****************************************************************************
** Function name:       Pcd_ConfigISOType
** Descriptions:        ���ò���Э��
** input parameters:    type 0��ISO14443AЭ�飻
					        1��ISO14443BЭ�飻
** output parameters:   None
** Returned value:	  OK
** Author:              quqian
*****************************************************************************/
uint8_t Pcd_ConfigISOType(uint8_t type) 
{
    if (type == 0)                     //ISO14443_A
    {
        Set_BitMask(ControlReg, 0x10); //ControlReg 0x0C ����readerģʽ
        Set_BitMask(TxAutoReg, 0x40); //TxASKReg 0x15 ����100%ASK��Ч
        Write_Reg(TxModeReg, 0x00);  //TxModeReg 0x12 ����TX CRC��Ч��TX FRAMING =TYPE A  ��������Ϊ106Kbs
        Write_Reg(RxModeReg,
                  0x00); //RxModeReg 0x13 ����RX CRC��Ч��RX FRAMING =TYPE A	  ��������Ϊ106Kbs���������ڽ���һ������֡���ٽ���


        Set_BitMask(0x39, 0x80);       //TestDAC1Reg�Ĵ���ΪTestDAC1�������ֵ

        Clear_BitMask(0x3C, 0x01);   //��ղ��ԼĴ�������

        Clear_BitMask(0x3D, 0x07);    //��ղ��ԼĴ�������

        Clear_BitMask(0x3E, 0x03);     //��ղ��ԼĴ�������


        Write_Reg(0x33, 0xFF);//TestPinEnReg�Ĵ���D1~D7�����������ʹ��

        Write_Reg(0x32, 0x07);

        Write_Reg(GsNOnReg, 0xF1);  //ѡ��������������TX1��TX2�絼��
        Write_Reg(CWGsPReg, 0x3F);  //ѡ��������������TX1��TX2�絼��
        Write_Reg(ModGsPReg, 0x01); //ѡ��������������TX1��TX2�絼��
        Write_Reg(RFCfgReg, 0x40);    //����Bit6~Bit4Ϊ100 ��������33db
        Write_Reg(DemodReg, 0x0D);
        Write_Reg(RxThresholdReg, 0x84);//0x18�Ĵ���	Bit7~Bit4 MinLevel Bit2~Bit0 CollLevel

        Write_Reg(AutoTestReg, 0x40);//AmpRcv=1
    }
    if (type == 1)                     //ISO14443_B
    {
        Write_Reg(ControlReg, 0x10); //ControlReg 0x0C ����readerģʽ
        Write_Reg(TxModeReg, 0x83); //TxModeReg 0x12 ����TX CRC��Ч��TX FRAMING =TYPE B
        Write_Reg(RxModeReg, 0x83); //RxModeReg 0x13 ����RX CRC��Ч��RX FRAMING =TYPE B
        Write_Reg(GsNOnReg, 0xF4); //GsNReg 0x27 ����ON�絼
        Write_Reg(GsNOffReg, 0xF4); //GsNOffReg 0x23 ����OFF�絼
        Write_Reg(TxAutoReg, 0x00);// TxASKReg 0x15 ����100%ASK��Ч
    }
    if (type == 2)                     //Felica
    {
        Write_Reg(ControlReg, 0x10); //ControlReg 0x0C ����readerģʽ
        Write_Reg(TxModeReg, 0x92); //TxModeReg 0x12 ����TX CRC��Ч��212kbps,TX FRAMING =Felica
        Write_Reg(RxModeReg, 0x96); //RxModeReg 0x13 ����RX CRC��Ч��212kbps,Rx Multiple Enable,RX FRAMING =Felica
        Write_Reg(GsNOnReg, 0xF4); //GsNReg 0x27 ����ON�絼
        Write_Reg(CWGsPReg, 0x20); //
        Write_Reg(GsNOffReg, 0x4F); //GsNOffReg 0x23 ����OFF�絼
        Write_Reg(ModGsPReg, 0x20);
        Write_Reg(TxAutoReg, 0x07);// TxASKReg 0x15 ����100%ASK��Ч
    }

    return OK;
}


uint8_t FM175XX_HardReset(void) 
{
    NPD_LOW();
    CARD_DELAY_MS(100);
    NPD_HIGH();
    CARD_DELAY_MS(100);

    return BswDrv_FM175XX_Check();
}

int BswDrv_FM175XX_Check(void)
{
    uint8_t reg_data = 0xFF;
    reg_data = Read_Reg(CommandReg);
    if (reg_data == Idle)
	{
		return OK;
	}
    else
	{
		return ERROR;
	}
}

int BswDrv_FM175XX_Init(void)
{
	//�򿪵�Դ
    FM1752_POWER_ON();
    
    //CPDN  �ߵ�ƽ
    NPD_HIGH();
	
    //CS �ߵ�ƽ
    SPI_CS_HIGH();
	
    if(FM175XX_HardReset() == OK)
    {
        //���ÿ�����Э��
        Pcd_ConfigISOType(0);
        //������Ƶ���-3��TX1��TX2�������TX2Ϊ�������
        Set_Rf(3);
        
        return CL_OK;
    }
    
    return CL_FAIL;
}

/*****************************************************************************
** Function name:       TestSPI_Interface
** Descriptions:        
** input parameters:    None
** output parameters:   None
** Returned value:	  None
** Author:              quqian
*****************************************************************************/
void TestSPI_Interface(void)
{
    uint8_t value=0xFF;
	
    Clear_BitMask(TxModeReg, 0xff);
    Set_BitMask(TxModeReg, 0x55);
    value = Read_Reg(TxModeReg);
    Write_Reg(TxModeReg, value | 0x22);
    value = Read_Reg(TxModeReg);
}


