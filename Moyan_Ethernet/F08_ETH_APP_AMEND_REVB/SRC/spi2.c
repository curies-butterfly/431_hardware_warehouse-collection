#include "config.h"
#include "socket.h"
#include "w5500.h"
#include "ult.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "DSP2833x_Device.h"
uint8_t SPI_I2S_ReceiveData(void);
void set_default(void);

void WIZ_SPI_Init(void)//Bluetooth
{

}

void WIZ_CS(uint8_t val)
{
	if (val == 0)
		{
			GpioDataRegs.GPADAT.bit.GPIO19 = 0;
		}
		else if (val == 1)
		{
			GpioDataRegs.GPADAT.bit.GPIO19 = 1;
		}
}


uint8_t SPI2_SendByte(uint8_t byte)
{
	uint8 SPIRXD;
	while(SpiaRegs.SPISTS.bit.BUFFULL_FLAG == 1);
	SpiaRegs.SPITXBUF=(byte<<8);
	while(SpiaRegs.SPISTS.bit.INT_FLAG != 1);{}//Wait until data is received
	SPIRXD = SpiaRegs.SPIRXBUF;
	return SPIRXD;

}




