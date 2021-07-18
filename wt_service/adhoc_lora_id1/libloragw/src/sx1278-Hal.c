#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include "sx1278-Hal.h"
#include "loragw_spi.h"
#include "loragw_hal.h"

static void *spi_target = NULL; 
#define GPIO_PATH "/sys/class/gpio"

void gpio_open(void);

void SX1278_Init_IO(void)
{
	lgw_spi_open(&spi_target, 0x1278);
	gpio_open();
}

//建立GPIO文件夹960，并将GPIO设置为输出1
void gpio_open(void)
{
	int valuefd, exportfd, directionfd;
 
    exportfd = open("/sys/class/gpio/export", O_WRONLY);
    if (exportfd < 0)
    {
        printf("Cannot open GPIO to export.\n");
		close(exportfd);
        exit(1);
    }
    write(exportfd, "960", 4);	//向export中写入960建立gpio960文件夹，创建gpio相关文件
    close(exportfd);
 
    directionfd = open("/sys/class/gpio/gpio960/direction", O_RDWR);
    if (directionfd < 0)
    {
        printf("Cannot open GPIO direction.\n");
		close(directionfd);
        exit(1);
    }
    write(directionfd, "out", 4);	//设置GPIO为输出
    close(directionfd);

	valuefd = open("/sys/class/gpio/gpio960/value", O_RDWR);
    if (valuefd < 0)
    {
        printf("Cannot open GPIO value.\n");
		close(valuefd);
        exit(1);
    }
	write(valuefd,"1", 2);
	close(valuefd);
}

void gpio_set_high(void)
{
	int valuefd = 0;
	valuefd = open("/sys/class/gpio/gpio960/value", O_RDWR);
    if (valuefd < 0)
    {
        printf("Cannot open GPIO value.\n");
        exit(1);
    }
	write(valuefd,"1", 2);
	close(valuefd);
}

void gpio_set_low(void)
{
	int valuefd = 0;
	valuefd = open("/sys/class/gpio/gpio960/value", O_RDWR);
    if (valuefd < 0)
    {
        printf("Cannot open GPIO value.\n");
        exit(1);
    }
	write(valuefd,"0", 2);
	close(valuefd);
}

//低电平复位有效
void SX1278SetReset(uint8_t state)                                  
{
	if(state == RADIO_RESET_ON )
	{	   
		gpio_set_low();
	}
	else
	{
		gpio_set_high();
	}
}

void SX1278WriteBuffer( uint8_t addr, uint8_t *buffer, uint8_t size ,uint8_t module )  
{
	lgw_spi_wb(spi_target, 0, 0, addr, buffer, size);
}

void SX1278Write( uint8_t addr, uint8_t data ,uint8_t module)                         
{
  	lgw_spi_w(spi_target, 0, 0, addr, data);
}

void SX1278WriteFifo( uint8_t *buffer, uint8_t size ,uint8_t module)                  
{
   	lgw_spi_wb(spi_target, 0, 0, 0, buffer, size);
}

void SX1278ReadBuffer( uint8_t addr, uint8_t *buffer, uint8_t size ,uint8_t module)   
{
	lgw_spi_rb(spi_target, 0, 0, addr, buffer, size);
}

void SX1278Read( uint8_t addr, uint8_t *data ,uint8_t module)  
{
   	lgw_spi_r(spi_target, 0, 0, addr, data);
}

void SX1278ReadFifo( uint8_t *buffer, uint8_t size ,uint8_t module)  
{
   	lgw_spi_rb(spi_target, 0, 0, 0, buffer, size);
}
/* --- EOF ------------------------------------------------------------------ */

