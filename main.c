#include <reg51.h>
#include <stdio.h>

//display pins
#define LCD_PORT P1
sbit LCD_E = P1^3;
sbit LCD_RS = P1^2;

//display variables
unsigned char * singleChar;
signed int temp;
char display_buffer[33];
float ftemp;

//sensor 
unsigned char sensorBuffer[2];
sbit pin_1W=P3^3;

//relay
sbit pin_relay = P3^7;

//function headers
void delay(unsigned int i);
void init_1W(void);
unsigned char readByte_1W(void);
void writeByte_1W(unsigned char dat);
void read_sensor(void);
void lcd_cmd(unsigned char cmd);
void lcd_chars(unsigned char * chars);
void lcd_init();

void main()
{
	pin_relay = 0;
	LCD_PORT = 0;
	LCD_E = 0;
	LCD_RS = 0;
	lcd_init();

	while(1)
	{
		read_sensor();
		temp = sensorBuffer[0]; //lsb
		temp &= 0x00FF;
		temp |= (sensorBuffer[1] << 8);
		ftemp = temp * 0.0625;
		sprintf(display_buffer, "T = %.2f", ftemp);
		lcd_cmd(0x01);
		lcd_cmd(0x80);
		lcd_chars(display_buffer);
		pin_relay = 1;
		delay(10000);
	}
}

//function implementations
void delay(unsigned int i)
{
    while(i--);
}

//OneWire basic functions
void init_1W(void)
{
    unsigned char x=0;
    pin_1W = 1;
    delay(8);
    pin_1W = 0;
    delay(80);
    pin_1W = 1;
    delay(14);
    x=pin_1W;
    delay(20);
}

unsigned char readByte_1W(void)
{
    unsigned char i=0;
    unsigned char dat = 0;
    for (i=8;i>0;i--)
    {
      pin_1W = 0;
      dat>>=1;
      pin_1W = 1;
      if(pin_1W)
      dat|=0x80;
      delay(4);
    }
    return(dat);
}

void writeByte_1W(unsigned char dat)
{
    unsigned char i=0;
    for (i=8; i>0; i--)
    {
      pin_1W = 0;
      pin_1W = dat&0x01;
      delay(5);
      pin_1W = 1;
      dat>>=1;
    }
    delay(4);
}

//temperature read for DS18B20
//raw 16 bit value in sensorBuffer
void read_sensor(void)
{
    init_1W();
    writeByte_1W(0xCC); //skip ROM
    writeByte_1W(0x44); //convert T
    init_1W();
    writeByte_1W(0xCC); //skip ROM
    writeByte_1W(0xBE); //read scratchpad
    sensorBuffer[0]=readByte_1W();
    sensorBuffer[1]=readByte_1W();
}

//LM016L compatible LCD display functions
void lcd_cmd(unsigned char cmd)
{
		LCD_RS = 0;
		LCD_PORT &= 0x0F; 
		LCD_PORT = (cmd & 0xF0);
		LCD_E = 1;
		delay(1000);
		LCD_E = 0;
	
		delay(10000);
	
		LCD_PORT &= 0x0F;
		LCD_PORT = ((cmd << 4) & 0xF0);
		LCD_E = 1;
		delay(1000);
		LCD_E = 0;
	
		delay(10000);
}

void lcd_chars(unsigned char * chars)
{
	LCD_RS = 1;
	singleChar = chars;
	
	while(*singleChar != '\0')
	{
		LCD_PORT &= 0x0F; 
		LCD_PORT |= ((*singleChar) & 0xF0);
		LCD_RS = 1;
		LCD_E = 1;
		delay(1000);
		LCD_E = 0;
		
		delay(10000);
		
		LCD_PORT &= 0x0F;
		LCD_PORT |= (((*singleChar) << 4) & 0xF0);
		LCD_RS = 1;
		LCD_E = 1;
		delay(1000);
		LCD_E = 0;
			
		delay(10000);
			
		singleChar++;
	}
}

void lcd_init()
{
	lcd_cmd(0x02); //4 bit mode
	lcd_cmd(0x28); //5x7 chars
	lcd_cmd(0x0E); //cursor on
	lcd_cmd(0x01); //clear
	lcd_cmd(0x80); //move cursor to first position
}