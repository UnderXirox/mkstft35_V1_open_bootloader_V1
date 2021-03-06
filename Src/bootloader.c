#include "bootloader.h"
#include "main.h"
#include "fatfs.h"
#include "board.h"
#include "flash.h"
#include "w25qxx.h"

#include "stm32_adafruit_lcd.h"
#include "stm32_adafruit_ts.h"
#include "ts.h"
#include "lcd_driver.h"
FATFS SDFatFS; /* File system object for SD logical drive */
extern SD_HandleTypeDef hsd;
char SDPath[4];
extern w25qxx_t w25qxx;
typedef void (*pFunction)(void);
extern SPI_HandleTypeDef hspi1;
static FILINFO File_info;
char txt_buf[70];
void dump_w25qxx();


void firmware_run(void) {
    
   // printf("\n\rworking with un-encrypted mks firmware_run\n\r");
    uint32_t appStack = (uint32_t) *((uint32_t *) APP_ADDR);

    pFunction appEntry = (pFunction) *(uint32_t *) (APP_ADDR + 4);
  #ifdef DEBUG
    printf("Stack Address %x \r\n",APP_ADDR);
    printf("Stack Address Value 0x%x \r\n",appStack);
    printf("Jump Address %x\r\n",(APP_ADDR+4));
    printf("Jump Address Value 0x%x\r\n",appEntry);
 #endif
    //HAL_Delay(300);
    HAL_SD_MspDeInit(&hsd);
    HAL_RCC_DeInit();
    HAL_DeInit();
  
    SCB->VTOR = APP_ADDR& 0x1FFFFF80;;
    SysTick->CTRL = 0;
    __set_MSP(appStack);
 
    appEntry();
}

static void old_MX_SPI1_Init(void)
{

 
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_HARD_OUTPUT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE END SPI1_Init 2 */
  __HAL_RCC_GPIOB_CLK_ENABLE();
  GPIO_InitStruct.Pin = GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_MODE_AF_PP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

FRESULT check_fName(const char *fname)
{
  FRESULT res = f_stat(fname, &File_info);
  printf("fname is  %s result %d\r\n",fname,res);
   	if (res != FR_OK)
		return FLASH_FILE_NOT_EXISTS;
    return 0;
}

void bootloader_start()
{

  unsigned char result;
  unsigned char line;
  //open device check, using dev pointer & path to sdcard store
  time_t rawtime = LAST_BUILD_TIME;
    struct tm  ts;
    char       buf[80];

    // Format time, "ddd yyyy-mm-dd hh:mm:ss zzz"
    ts = *localtime(&rawtime);
    //Full time date strftime(buf, sizeof(buf), "%a %Y-%m-%d %H:%M:%S %Z", &ts);
    #if 1
    strftime(buf, sizeof(buf), "Build Time:  %a %Y-%m-%d %H:%M:%S %Z", &ts);
    #else
    strftime(buf, sizeof(buf), "Build Time: %m-%d %H:%M %Z", &ts);
    #endif
    printf("%s\r\n", buf);

    result = f_mount(&SDFatFS, SDPath, 1);
 
    if (!result == FR_OK)
    {
      BSP_LCD_SetTextColor(LCD_COLOR_GREEN);
      snprintf(txt_buf,50,"%s",BOOTLOAD_SPLASH);
      BSP_LCD_SetTextColor(LCD_COLOR_GREEN);
     BSP_LCD_DisplayStringAt(1,12,(uint8_t *)txt_buf,LEFT_MODE);
      snprintf(txt_buf,30,"Bootloader Version %s",BOOTLOADER_VERSION);
      BSP_LCD_SetTextColor(LCD_COLOR_YELLOW);
     BSP_LCD_DisplayStringAt(1,12*2+1,(uint8_t *)txt_buf,LEFT_MODE);
      snprintf(txt_buf,70,"Patrons:- %s",BOOTLOADER_PATRONS);
      BSP_LCD_SetTextColor(LCD_COLOR_GREEN);
     BSP_LCD_DisplayStringAt(1,12*3+1,(uint8_t *)txt_buf,LEFT_MODE);

    /*TFT_DrawString((9*32),0,BOOTLOAD_SPLASH,Green);
    TFT_DrawString((9*32),320,BOOTLADER_VERSION,Yellow);
    TFT_DrawString((9*33),0,"Patrons:",LightGrey);
    TFT_DrawString((9*33),66,BOOTLOADER_PATRONS,Yellow);
    TFT_DrawString((9*34),00,buf,Green);
   */
    }
    if (result == FR_OK)
  {
  
    /*
    Check for vanilla firmware first 
    */
      if (check_fName(VANILLA_CURR)==0)
      {
        printf("SD Open Success Flashing Vanilla\r\n");
        line=2;
        snprintf(txt_buf,30,"SD Open Success Flashing Vanilla");
        BSP_LCD_SetTextColor(LCD_COLOR_GREEN);
        BSP_LCD_DisplayStringAt(1,LCD_HT-(12*2),(uint8_t *)txt_buf,LEFT_MODE);
       // TFT_DrawString((9*line),0,"SD Open Success Flashing Vanilla" ,Green);
        FlashResult Flash_status = flash(VANILLA_CURR);
      }
      else 
      if (check_fName(CURR_NAME)==0)
      {
        printf("SD Open Success Flashing MKSRobin Crypto\r\n");
        line=2;
     //   TFT_DrawString((9*line),0,"SD Open Success Flashing MKSRobin Crypto" ,Green);
        FlashResult Flash_status = flash_crypt(CURR_NAME);
      }
      
      FlashResult Flash_status = 0;
      //flash_crypt(CURR_NAME);
      line=4;
     // printf("flash result %d\r\n",Flash_status);
      if (Flash_status == 0)
      {
        //  TFT_DrawString((9*line),0,"Flash Success" ,Green);
        }
      else 
      {
        //  TFT_DrawString((9*line),0,"Flash Fail" ,Red);
        }
  } 
  else {
      line=4;
      snprintf(txt_buf,30,"SD Open 0:/");
      BSP_LCD_SetTextColor(LCD_COLOR_GREEN);
       BSP_LCD_DisplayStringAt(1,LCD_HT-(12*2),(uint8_t *)txt_buf,LEFT_MODE);
       snprintf(txt_buf,30,"Fail - SD Card not found/file not found");
      BSP_LCD_SetTextColor(LCD_COLOR_RED);
       BSP_LCD_DisplayStringAt(100,LCD_HT-(12*2),(uint8_t *)txt_buf,LEFT_MODE);
    //   TFT_DrawString((9*line),0,"SD Open 0:/" ,Green);
    //   TFT_DrawString((9*line),95,"Fail - SD Card not found/file not found" ,Red);
       }
line=5;
//TFT_DrawString((9*line),0,"*****   Booting Firmware ******" ,Green);
snprintf(txt_buf,30,"*****   Booting Firmware ******");
BSP_LCD_SetTextColor(LCD_COLOR_GREEN);
BSP_LCD_DisplayStringAt(1,LCD_HT-(12*10),(uint8_t *)txt_buf,CENTER_MODE);

printf("Init w25qxx device\r\n");
//MX_SPI1_Init();
//HAL_Delay(3000);
//TFT_Switch_Init();
W25qxx_Init();
//HAL_Delay(3000);
//TFT_Clear(Black);
int pic_cnt=0;
 DRAW_LOGO();
 printf("logo done");
while(0)
{
 
  HAL_Delay(3000);
// test_pic();
 
 
}
//printf("w25qxx ID %d\r\n",W25qxx_ReadID());
/*while (1)
{
  HAL_Delay(100);
  HAL_GPIO_WritePin(GPIOB,GPIO_PIN_9,GPIO_PIN_RESET);
  HAL_Delay(100);
  HAL_GPIO_WritePin(GPIOB,GPIO_PIN_9,GPIO_PIN_SET);
  printf("gpio pin toggle");
}*/
//dump_w25qxx();
//printf("halt debug");
//HAL_Delay(1000);
//debug halt 
//while(1);

firmware_run();
  
  while(1)
  {
      //should not be here part duex the smell of failure 
  }
}



uint8_t buffer[60];

void dump_w25qxx()
{
   // W25qxx_ReadBytes(uint8_t *pBuffer, uint32_t ReadAddr, uint32_t NumByteToRead);
    //HAL_GPIO_WritePin(GPIOB,GPIO_PIN_9,GPIO_PIN_RESET);
   
   
    W25qxx_ReadBytes(buffer,0x0,sizeof(buffer));
    printf("\r\n Dump eeprom:-\r\n");
    for (int pic_cnt=0;pic_cnt<sizeof(buffer);pic_cnt++)
    {
        printf("0x%x ",buffer[pic_cnt]);
        
    }
    printf("end");
}