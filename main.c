/**
* Room thermostat based on the 8051 microcontroller.
* Temperature sensor : DS18B20
* Developed by Stelian Saracut
* 2018
*
*/

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
float currentTemp, setTemp;

//sensor
unsigned char sensorBuffer[2];
sbit pin_1W=P3^3;

//relay
sbit pin_relay = P3^7;

//keyboard
sbit key_dec = P1^0;
sbit key_inc = P1^1;

//function headers
void delay(unsigned int i);
void init_1W(void);
unsigned char readByte_1W(void);
void writeByte_1W(unsigned char dat);
void read_sensor(void);
void lcd_cmd(unsigned char cmd);
void lcd_chars(unsigned char * chars);
void lcd_init();
void get_keys();

void main()
{
	// I/O pins initialization
	pin_relay = 0;
	LCD_PORT = 0x0F;
	LCD_E = 0;
	LCD_RS = 0;
	key_dec = 1;
	key_inc = 1;
	setTemp = 21.0f;
	
	//LCD initialization
	lcd_init();

	while(1)
	{
		read_sensor();
		temp = sensorBuffer[0]; //lsb
		temp &= 0x00FF;
		temp |= (sensorBuffer[1] << 8);
		//convert raw sensor data to temperature
		currentTemp = temp * 0.0625;
		//format line 1
		sprintf(display_buffer, "T     = %.2f", currentTemp);
		lcd_cmd(0x01);
		lcd_cmd(0x80);
		//display line 1
		lcd_chars(display_buffer);
		//move to line 2
		lcd_cmd(0xC0);
		sprintf(display_buffer, "Set T = %.2f", setTemp);
		//display line 2
		lcd_chars(display_buffer);
		//enable relay if current temperature is lower
		pin_relay = (setTemp > currentTemp);
		//read keyboard
		get_keys();
		delay(10);
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
		LCD_PORT |= (cmd & 0xF0);
		LCD_E = 1;
		delay(100);
		LCD_E = 0;

		delay(1000);

		LCD_PORT &= 0x0F;
		LCD_PORT |= ((cmd << 4) & 0xF0);
		LCD_E = 1;
		delay(100);
		LCD_E = 0;

		delay(1000);
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
		delay(100);
		LCD_E = 0;

		delay(1000);

		LCD_PORT &= 0x0F;
		LCD_PORT |= (((*singleChar) << 4) & 0xF0);
		LCD_RS = 1;
		LCD_E = 1;
		delay(100);
		LCD_E = 0;

		delay(1000);

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

//reads and debounces keys and modifies set temperature
void get_keys()
{
	key_inc = 1;
	key_dec = 1;
	
	if(!key_inc)
	{
			delay(1000);
			if (!key_inc)
				setTemp += 0.5f;
			return;
	}
	
	if(!key_dec)
	{
			delay(1000);
			if (!key_dec)
				setTemp -= 0.5f;
			return;
	}
}