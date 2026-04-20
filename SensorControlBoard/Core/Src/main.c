/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2023 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "i2c.h"
#include "iwdg.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <math.h>
#include <string.h>
#include "chry_ringbuffer.h"
#include "stdio.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define SYS_LOG_OUT 1
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

#define DIGITAL_SCALE_12BITS ((uint32_t)0xFFF)

/* Init variable out of ADC expected conversion data range */
#define VAR_CONVERTED_DATA_INIT_VALUE (DIGITAL_SCALE_12BITS + 1)

/* Definition of ADCx conversions data table size */
#define ADC_CONVERTED_DATA_BUFFER_SIZE ((uint32_t)2)
/* Variables for ADC conversion data */
__IO uint16_t aADCxConvertedData[ADC_CONVERTED_DATA_BUFFER_SIZE]; /* ADC group regular conversion data (array of data) */

struct __statusData
{
  uint8_t cam_power;
  uint8_t fan_power;
  uint8_t led_power;
  uint8_t alarm_timUP;
  uint16_t AHT20_humidity; // 100.00  ->2
  uint16_t AHT20_temperature;
  uint16_t NTC1_temperature;
  uint16_t NTC2_temperature;
} sysStatusData;

uint8_t powerControlFunc()
{
  static uint8_t f_cam_power = 1, f_fan_power = 0, f_led_power = 0;

  if (sysStatusData.cam_power != f_cam_power)
  {
    if (sysStatusData.cam_power)
    {
      HAL_GPIO_WritePin(CAM_CTRL_GPIO_Port, CAM_CTRL_Pin, GPIO_PIN_SET);
    }
    else
    {
      HAL_GPIO_WritePin(CAM_CTRL_GPIO_Port, CAM_CTRL_Pin, GPIO_PIN_RESET);
    }
  }
  if (sysStatusData.led_power != f_led_power)
  {
    // if (0 == sysStatusData.led_power)
    // {
    //   HAL_GPIO_WritePin(LED_CTRL_GPIO_Port, LED_CTRL_Pin, GPIO_PIN_SET);
    // }
    // else
    // {
    //   HAL_GPIO_WritePin(LED_CTRL_GPIO_Port, LED_CTRL_Pin, GPIO_PIN_RESET);
    // }
    // 10Hz
    if (0 == sysStatusData.led_power)
    {
      // 100%
      __HAL_TIM_SET_AUTORELOAD(&htim16, 999);
      __HAL_TIM_SET_COMPARE(&htim16, TIM_CHANNEL_1, 998);
    }
    else if (1 == sysStatusData.led_power)
    {
      // 0%
      __HAL_TIM_SET_COMPARE(&htim16, TIM_CHANNEL_1, 0);
    }
    else if (2 == sysStatusData.led_power)
    {
      // 5Hz 50%
      __HAL_TIM_SET_AUTORELOAD(&htim16, 1000 * 2 - 1);
      __HAL_TIM_SET_COMPARE(&htim16, TIM_CHANNEL_1, 1000);
    }
    else if (3 == sysStatusData.led_power)
    {
      // 1Hz  50%
      __HAL_TIM_SET_AUTORELOAD(&htim16, 10000 - 1);
      __HAL_TIM_SET_COMPARE(&htim16, TIM_CHANNEL_1, 5000);
    }
    else if (4 == sysStatusData.led_power)
    {
      // 0.5Hz 10%
      __HAL_TIM_SET_AUTORELOAD(&htim16, 10000 * 2 - 1);
      __HAL_TIM_SET_COMPARE(&htim16, TIM_CHANNEL_1, 2000);
    }
  }
  if (sysStatusData.fan_power != f_fan_power)
  {
    if (sysStatusData.fan_power >= 9)
      sysStatusData.fan_power = 9;

    __HAL_TIM_SET_COMPARE(&htim17, TIM_CHANNEL_1, 10 * sysStatusData.fan_power);
  }

  f_cam_power = sysStatusData.cam_power;
  f_fan_power = sysStatusData.fan_power;
  f_led_power = sysStatusData.led_power;

  return 1;
}
uint8_t BLINK_SYS_LED(uint16_t Tcounter)
{
  static uint16_t tick = 0;

  if (tick >= Tcounter)
  {
    HAL_GPIO_TogglePin(BLINK_GPIO_Port, BLINK_Pin);
    HAL_GPIO_TogglePin(BLINK2_GPIO_Port, BLINK2_Pin);
    tick = 0;
#if SYS_LOG_OUT
    printf("\r\n-----SYS INFO-----\r\n");
    printf("sysStatusData.cam_power=%d\n", sysStatusData.cam_power);
    printf("sysStatusData.fan_power=%d\n", sysStatusData.fan_power);
    printf("sysStatusData.led_power=%d\n", sysStatusData.led_power);
    printf("sysStatusData.AHT20_temperature=%d.%d\n", sysStatusData.AHT20_temperature / 100, sysStatusData.AHT20_temperature % 100);
    printf("sysStatusData.AHT20_humidity=%d.%d\n", sysStatusData.AHT20_humidity / 100, sysStatusData.AHT20_humidity % 100);
    printf("sysStatusData.NTC1_temperature=%d.%d\n", sysStatusData.NTC1_temperature / 100, sysStatusData.NTC1_temperature % 100);
    printf("sysStatusData.NTC2_temperature=%d.%d\n", sysStatusData.NTC2_temperature / 100, sysStatusData.NTC2_temperature % 100);
#endif
  }
  else
  {
    tick++;
  }

  return 1;
}
void ADC_APP_Init()
{
  /* Run the ADC calibration */
  if (HAL_ADCEx_Calibration_Start(&hadc1) != HAL_OK)
  {
    /* Calibration Error */
    Error_Handler();
  }

  /*## Start ADC conversions ###############################################*/
  /* Start ADC group regular conversion with DMA */
  if (HAL_ADC_Start_DMA(&hadc1,
                        (uint32_t *)aADCxConvertedData,
                        ADC_CONVERTED_DATA_BUFFER_SIZE) != HAL_OK)
  {
    /* ADC conversion start error */
    Error_Handler();
  }

  //  while (1)
  //  {
  //    HAL_Delay(1000);
  //    printf("aADCxConvertedData[0]=%d,vo1=%.2f\r\n", aADCxConvertedData[0], aADCxConvertedData[0] * 3.3f / 4095.0);
  //    printf("aADCxConvertedData[1]=%d,vo2=%.2f\r\n", aADCxConvertedData[1], aADCxConvertedData[1] * 3.3f / 4095.0);
  //  }
}

/**
Bֵ��
v/3.3=R/(R+10K)
**/
uint8_t ADCTempTransfer_NTC(void)
{
  const float Rp = 10000.0f; // 10K
  const float T2 = (273.15f + 25.0f);
  ;                         // T2
  const float Bx = 3950.0f; // B
  const float Ka = 273.15f;
  float Rt;
  float temp;
  float mvoltage[2];
  for (uint8_t i = 0; i < 2; i++)
  {
    mvoltage[i] = aADCxConvertedData[i] * 3.3f / 4095.0;
    Rt = 10000.0 * mvoltage[i] / 3.3 / (1.0 - mvoltage[i] / 3.3);
    //	Rt = Get_TempResistor();
    // like this R=5000, T2=273.15+25,B=3470, RT=5000*EXP(3470*(1/T1-1/(273.15+25)),
    temp = Rt / Rp;
    temp = log(temp); // ln(Rt/Rp)
    temp /= Bx;       // ln(Rt/Rp)/B
    temp += (1.0 / T2);
    temp = 1.0 / (temp);
    temp -= Ka;

    if ((uint16_t)(temp) >= 500)
      continue;
    if (0 == i)
    {

      sysStatusData.NTC1_temperature = (uint16_t)(temp * 100);
    }
    else if (1 == i)
    {
      sysStatusData.NTC2_temperature = (uint16_t)(temp * 100);
    }
  }

  return 1;
}
void I2C_Device_Identity()
{
  while (1)
  {
    printf("\nI2C init\n");
    uint8_t data[10];
    for (uint8_t i = 0; i < 255; i++)
    {
      uint8_t re = HAL_I2C_Mem_Read(&hi2c1, i, 0, I2C_MEMADD_SIZE_8BIT, data, 1, 0xff);
      if (re == HAL_OK)
        printf("0x%02x ", i);
    }
    printf("\nI2C finish\n");
    HAL_Delay(500);
  }
}

uint8_t CheckCrc8(uint8_t *pDat, uint8_t Lenth)
{
  __IO uint8_t crc = 0xff, i, j;

  for (i = 0; i < Lenth; i++)
  {
    crc = crc ^ *pDat;
    for (j = 0; j < 8; j++)
    {
      if (crc & 0x80)
        crc = (crc << 1) ^ 0x31;
      else
        crc <<= 1;
    }
    pDat++;
  }
  return crc;
}
#define I2C_AHT20_ADDRESS_W 0X70
#define I2C_AHT20_ADDRESS_R 0X71
void AHT20_READ_MACHINE()
{

  __IO uint8_t ReadByte[7];
  uint8_t DATA[2] = {0x33, 0x00};
  uint8_t re;
  uint32_t read_Data;
  float RH, TEMP;
  uint8_t sendbuffer[3] = {0xAC, 0x33, 0x00};
  uint8_t readBuffer[6];

  re = HAL_I2C_Master_Transmit(&hi2c1, I2C_AHT20_ADDRESS_W, sendbuffer, 3, HAL_MAX_DELAY);
  if (re == HAL_OK)
  {
    //    printf("OK:IDLE\n");
  }
  HAL_Delay(75);
  re = HAL_I2C_Master_Receive(&hi2c1, I2C_AHT20_ADDRESS_R, ReadByte, 7, HAL_MAX_DELAY);
  if (re == HAL_OK)
  {
    if ((ReadByte[0] & 0x80) == 0x00)
    {
      //      uint32_t data = 0;
      //      data = ((uint32_t)ReadByte[3] >> 4) + ((uint32_t)ReadByte[2] << 4) + ((uint32_t)ReadByte[1] << 12);
      //      RH = data * 100.0f / (1 << 20);
      //      data = (((uint32_t)ReadByte[3] & 0X0F) << 16) + ((uint32_t)ReadByte[4] << 8) + ((uint32_t)ReadByte[5]);
      //      // sysStatusData.AHT20_humidity = (uint16_t)(RH * 100);
      //      printf("RH=%f\r\n", RH);

      //      TEMP = data * 200.0f / (1 << 20) - 50;
      //      printf("TEMP=%f\r\n", TEMP);
      // sysStatusData.AHT20_temperature = (uint16_t)(TEMP * 100);

      if ((CheckCrc8(ReadByte, 6) == ReadByte[6]) && ((ReadByte[0] & 0x98) == 0x18))
      {
        read_Data = ReadByte[1] * 65536 + ReadByte[2] * 256 + ReadByte[3];
        RH = (read_Data >> 4) * 100.0 / 1048576;

        //        printf("RH2=%f\r\n", RH);
        sysStatusData.AHT20_humidity = (uint16_t)(RH * 100);
        read_Data = 0;
        read_Data = (ReadByte[3] & 0x0F) * 65536 + ReadByte[4] * 256 + ReadByte[5];
        TEMP = read_Data * 200.0 / 1048576 - 50;
        //        printf("TEMP2=%f\r\n", TEMP);
        sysStatusData.AHT20_temperature = (uint16_t)(TEMP * 100);
      }
    }
  }
}
#define AHT20_ADDRESS 0x70
// AHT20的初始化函数
void AHT20_Init()
{
  uint8_t readbuffer;                                                           // 用于接收状态信息
  HAL_Delay(40);                                                                // 延时40ms，因为第一步的上电后要等待40ms
  HAL_I2C_Master_Receive(&hi2c1, AHT20_ADDRESS, &readbuffer, 1, HAL_MAX_DELAY); // 上面介绍了函数的参数作用
  if ((readbuffer & 0x08) == 0x00)                                              //根据手册，首先要看状态字的校准使能位Bit[3]是否为 1(通过发送 0x71可以获取一个字节的状态字)，如果不为1，\
		 要发送0xBE命令(初 始化)，此命令参数有两个字节， 第一个字节为0x08，第二个字节 为0x00。
  {
    uint8_t sendbuffer[3] = {0xBE, 0x08, 0x00};
    HAL_I2C_Master_Transmit(&hi2c1, AHT20_ADDRESS, sendbuffer, 3, HAL_MAX_DELAY);
  }
}
void AHT20_Read(float *Temperature, float *Humidity) // 读取温湿度的函数
{
  uint8_t sendbuffer[3] = {0xAC, 0x33, 0x00};
  uint8_t readBuffer[6];

  HAL_I2C_Master_Transmit(&hi2c1, AHT20_ADDRESS, sendbuffer, 3, HAL_MAX_DELAY);

  HAL_Delay(75);

  HAL_I2C_Master_Receive(&hi2c1, AHT20_ADDRESS + 1, readBuffer, 6, HAL_MAX_DELAY);
  if ((readBuffer[0] & 0x80) == 0x00)
  {
    uint32_t data = 0;

    data = ((uint32_t)readBuffer[3] >> 4) + ((uint32_t)readBuffer[2] << 4) + ((uint32_t)readBuffer[1] << 12);
    *Humidity = data * 100.0f / (1 << 20);
    data = (((uint32_t)readBuffer[3] & 0X0F) << 16) + ((uint32_t)readBuffer[4] << 8) + ((uint32_t)readBuffer[5]);

    *Temperature = data * 200.0f / (1 << 20) - 50;
  }
}

chry_ringbuffer_t UART3rb;
uint8_t UART3mempool[128];
void UART3_Init()
{
  LL_USART_EnableIT_RXNE(USART3);
  /**
   * �????要注意的点是，init 函数第三个参数是内存池的大小（字节为单位�????
   * 也是ringbuffer的深度，必须�???? 2 的幂次！！！�????
   * 例如 4�????16�????32�????64�????128�????1024�????8192�????65536�????
   */
  if (0 == chry_ringbuffer_init(&UART3rb, UART3mempool, 128))
  {
    printf("UART3mempool success\r\n");
  }
  else
  {
    printf("UART3mempool error\r\n");
  }
}
void UART3_SendString(uint8_t *str)
{
  while (*str != '\0')
  {
    while (!LL_USART_IsActiveFlag_TXE(USART3))
      ;
    /* Echo received character on TX */
    LL_USART_TransmitData8(USART3, *str);
    str++;
  }
}

uint8_t uart_task_handle2()
{
  uint8_t data[128];
  uint8_t index = 0;
  uint8_t senddata[50];
  char tempcomArray[][5] = {"ON", "OFF"};
  char *ptr;
  uint8_t len = chry_ringbuffer_read(&UART3rb, data, 50);
  if (len)
  {
    printf("\r\n[C] read success, read %d byte\r\n", len);
    data[50] = '\0';
    printf("%s\r\n", data);
    if (strstr(data, "powerStatus") != NULL)
    {
      sprintf(senddata, "Status,%d,%d,%d,OK\n", sysStatusData.cam_power,
              sysStatusData.led_power, sysStatusData.fan_power);
      UART3_SendString(senddata);
      printf(senddata);
    }
    else if (strstr(data, "sensorData") != NULL)
    {
      sprintf(senddata, "SENSOR,%.1f,%.1f,%.1f,%.1f,OK\n", sysStatusData.NTC1_temperature / 100.0, sysStatusData.NTC2_temperature / 100.0,
              sysStatusData.AHT20_temperature / 100.0, sysStatusData.AHT20_humidity / 100.0);
      UART3_SendString(senddata);
      printf(senddata);
    }
    else if (strstr(data, "SET") != NULL)
    {
      ptr = strchr(data, 'A');
      if (ptr != NULL)
      {
        printf("set cam: %c\n", *(ptr + 1));
        sysStatusData.cam_power = *(ptr + 1) - '0';
      }
      ptr = strchr(data, 'B');
      if (ptr != NULL)
      {
        printf("set led: %c\n", *(ptr + 1));
        sysStatusData.led_power = *(ptr + 1) - '0';
      }

      // ptr=strchr(data,'C');
      // if(ptr != NULL){
      //   uint8_t temp= *(ptr+1)-'0';
      // 	printf("set fan: %d\n", temp);
      // 	sysStatusData.fan_power=temp;
      // }

      sprintf(senddata, "Status,%d,%d,%d,OK\n", sysStatusData.cam_power,
              sysStatusData.led_power, sysStatusData.fan_power);
      UART3_SendString(senddata);
      printf(senddata);
    }
  }

  if (1 == sysStatusData.alarm_timUP)
  {
    printf("\r\n1\r\n");
    sysStatusData.alarm_timUP = 2;
    sprintf(senddata, "Status,%d,%d,%d,OK\n", sysStatusData.cam_power,
            sysStatusData.led_power, sysStatusData.fan_power);
    UART3_SendString(senddata);
    printf(senddata);
  }
  else if (2 == sysStatusData.alarm_timUP)
  {
    printf("\r\n2\r\n");
    sysStatusData.alarm_timUP = 0;
    sprintf(senddata, "SENSOR,%.1f,%.1f,%.1f,%.1f,OK\n", sysStatusData.NTC1_temperature / 100.0, sysStatusData.NTC2_temperature / 100.0,
            sysStatusData.AHT20_temperature / 100.0, sysStatusData.AHT20_humidity / 100.0);
    UART3_SendString(senddata);
    printf(senddata);
  }

  // 加入迟滞比较器
  //  static uint8_t fan_run=0;
  uint8_t t_temp1 = sysStatusData.AHT20_temperature / 100;
  uint8_t t_temp2 = sysStatusData.NTC1_temperature / 100;
  uint8_t t_temp3 = sysStatusData.NTC2_temperature / 100;
  uint8_t tt_temperature = t_temp1 > t_temp2 ? t_temp1 : t_temp2;
  tt_temperature = tt_temperature > t_temp3 ? tt_temperature : t_temp3;
  if (tt_temperature >= 55)
  {
    //    fan_run &=0x2;
    sysStatusData.fan_power = 9;
  }
  else if (tt_temperature >= 45)
  {
    //    fan_run &=0x1;
    sysStatusData.fan_power = 5;
  }
  else if (tt_temperature <= 40)
  {
    //    //低2位清零
    //    fan_run &=0XFC;
    sysStatusData.fan_power = 0;
  }

  // else if(sysStatusData.AHT20_temperature / 100 <= 30)
  // {
  //   fan_run =0;
  // }

  /*
    if (sysStatusData.AHT20_temperature / 100 >= 40 || sysStatusData.NTC1_temperature / 100 >= 40 || sysStatusData.NTC2_temperature / 100 >= 40)
    {
      sysStatusData.fan_power = 1;
    }
    else if (sysStatusData.AHT20_temperature / 100 >= 50 || sysStatusData.NTC1_temperature / 100 >= 50 || sysStatusData.NTC2_temperature / 100 >= 50)
    {
      sysStatusData.fan_power = 9;
    }
    else
    {
      sysStatusData.fan_power = 0;
    }
  */

  return 1;
}

/*
AHT20温湿度读�????-》NTC温度解析-》串口数据解析与发�??-》电源控�????-》blink
*/
enum
{
  IDLE = 0,
  AHT20_READ_STATUS = 1,
  NTC_TEMP_READ_STATUS = 2,
  USART_DATA_ANALYSIS_STATUS = 3,
  POWER_CONTROL_STATUS = 4,
  BLINK_STATUS = 5
} MachineStatus;

void UART3_DEMO()
{
  char data[128];
  //	uart3ReceivedHandler();
  while (1)
  {
    uint32_t len = chry_ringbuffer_read(&UART3rb, data, 50);
    if (len)
    {
      printf("[C] read success, read %d byte\r\n", len);
      data[50] = '\0';
      printf("%s\r\n", data);
    }
    else
    {
      //            printf("[C] read faild, no data in ringbuffer\r\n");
    }
    //				UART3_SendString("HELLO\n");
    HAL_Delay(1000);
  }
}
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  static uint8_t tickUp = 0;
  if (htim->Instance == TIM3)
  {
    if (tickUp < 59)
      tickUp++;
    else
    {
      tickUp = 0;
      sysStatusData.alarm_timUP = 1;
    }
  }
}

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART1_UART_Init();
  MX_USART3_UART_Init();
  MX_ADC1_Init();
  MX_I2C1_Init();
  MX_TIM17_Init();
  MX_IWDG_Init();
  MX_TIM3_Init();
  MX_TIM16_Init();
  /* USER CODE BEGIN 2 */
  printf("\r\nsensor controller start\r\n");
  UART3_Init();
  //	UART3_DEMO();
  UART3_SendString("Restart\n");
  HAL_TIM_PWM_Start(&htim17, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim16, TIM_CHANNEL_1);
    uint8_t com_value=20;
  //  __HAL_TIM_SET_COMPARE(&htim17,TIM_CHANNEL_1,com_value);
  //  uartProtocolTransmit(0xf01,0xf00);
  //  I2C_Device_Identity();
  ADC_APP_Init();
  sysStatusData.cam_power = 1;
  sysStatusData.alarm_timUP = 0;
  HAL_TIM_Base_Start_IT(&htim3);
  sysStatusData.led_power = 1 ;
  //	sysStatusData.fan_power=50;
  //  float temp, hum;
  AHT20_Init();
  if (HAL_IWDG_Refresh(&hiwdg) != HAL_OK)
  {
    /* Refresh Error */
    Error_Handler();
  }
  //  while (1)
  //  {
  //    AHT20_Read(&temp, &hum);
  //    printf("temp=%.2f,hum=%.2f\r\n", temp, hum);
  //    HAL_Delay(1000);
  //  }
  __IO uint8_t aht20_read_delay = 0;
  //	while(1)
  //	{
  //	    HAL_GPIO_TogglePin(BLINK_GPIO_Port, BLINK_Pin);
  //    HAL_GPIO_TogglePin(BLINK2_GPIO_Port, BLINK2_Pin);
  //		HAL_Delay(100);
  //	}
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    /*
      AHT20_READ_STATUS=1,
      NTC_TEMP_READ_STATUS=2,
      USART_DATA_ANALYSIS_STATUS=3,
      POWER_CONTROL_STATUS=4,
      BLINK_STATUS=5
    */
    switch (MachineStatus)
    {
    case IDLE:
      MachineStatus = AHT20_READ_STATUS;

      break;
    case AHT20_READ_STATUS:
      HAL_Delay(25);
      //      AHT20_READ_MACHINE();
      aht20_read_delay++;
      if (aht20_read_delay >= 80)
      {
        aht20_read_delay = 0;
        AHT20_READ_MACHINE();
      }

      MachineStatus = NTC_TEMP_READ_STATUS;
      break;

    case NTC_TEMP_READ_STATUS:
      if (1 == ADCTempTransfer_NTC())
      {

        MachineStatus = USART_DATA_ANALYSIS_STATUS;
      }
      break;
    case USART_DATA_ANALYSIS_STATUS:
      if (1 == uart_task_handle2())
      {
        MachineStatus = POWER_CONTROL_STATUS;
      }
      break;
    case POWER_CONTROL_STATUS:
      if (1 == powerControlFunc())
      {
        MachineStatus = BLINK_STATUS;
      }
      break;
    case BLINK_STATUS:
      if (1 == BLINK_SYS_LED(30))
      {
        MachineStatus = AHT20_READ_STATUS;

        if (HAL_IWDG_Refresh(&hiwdg) != HAL_OK)
        {
          /* Refresh Error */
          Error_Handler();
        }
      }

      break;
    default:
      break;
    }
    //     HAL_GPIO_TogglePin(BLINK_GPIO_Port, BLINK_Pin);
    //     HAL_Delay(500);
    //     HAL_GPIO_TogglePin(MAINB_CTRL_GPIO_Port, MAINB_CTRL_Pin);
    //     HAL_Delay(500);
    //     HAL_GPIO_TogglePin(MAINB_CTRL_GPIO_Port, CAM_CTRL_Pin);
    //     HAL_Delay(500);
    //     HAL_GPIO_TogglePin(MAINB_CTRL_GPIO_Port, LED_CTRL_Pin);
    //     HAL_Delay(500);
    // //    __HAL_TIM_SET_COMPARE(&htim17,TIM_CHANNEL_1,com_value);
    //     if(com_value<90)
    //     {
    //       com_value+=10;
    //     }
    //     else
    //     com_value=0;
    //     HAL_Delay(500);
  }
  /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
   */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI | RCC_OSCILLATORTYPE_LSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV1;
  RCC_OscInitStruct.PLL.PLLN = 8;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
