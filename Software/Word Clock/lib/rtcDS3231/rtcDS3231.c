/* 
 * rtcDS3231.c
 * 
 * library for the DS3231 real time clock
 *
 * Created: 10/04/2017 15:15:46
 * Author : Heol
 */ 

#include <avr/io.h>

#include "rtcDS3231.h"

#define RTC_ADDR 0x68 // I2C address


uint8_t bcdtodec(uint8_t bcd)
{
	return ((bcd>>4)*10) + (bcd & 0x0F);
}

uint8_t dectobcd(uint8_t dec)
{
	return ((dec/10)<<4) + (dec % 10);
}

void rtc_set_time_24h(uint8_t hour, uint8_t min, uint8_t sec)
{
	i2c_start((RTC_ADDR<<1) | I2C_WRITE);
	i2c_write(0x00);
	
	i2c_write(dectobcd(sec));
	i2c_write(dectobcd(min));
	i2c_write(dectobcd(hour));
	
	i2c_stop();
}

void rtc_set_time_12h(uint8_t hour, uint8_t min, uint8_t sec, uint8_t am_pm)
{
	uint8_t temphour = 0;
	
	if (hour >= 12) return;
	
	if (am_pm)	// if PM
	temphour = 0x40 | 0x20 | dectobcd(hour);
	else        // if AM
	temphour = 0x40 | dectobcd(hour);
	
	i2c_start((RTC_ADDR<<1) | I2C_WRITE);
	i2c_write(0x00);
	i2c_write(dectobcd(sec));
	i2c_write(dectobcd(min));
	i2c_write(temphour);
	
	i2c_stop();
}

void rtc_set_date(uint16_t year, uint8_t month, uint8_t date)
{
	uint8_t century = 0;
	
	if((year / 100) == 20)
	{
		year = year - 2000;
		century = 1;
	}
	else
	{ 
		if((year / 100) == 19)
		{
			year = year - 1900;
			century = 0;
		}
	}
	
	i2c_start((RTC_ADDR<<1) | I2C_WRITE);
	i2c_write(0x04);
	
	i2c_write(dectobcd(date));
	i2c_write((century << 7) | dectobcd(month));
	i2c_write(dectobcd(year));
	
	i2c_stop();
}

void rtc_get_time_24h(uint8_t *hour, uint8_t *min, uint8_t *sec)
{
	// 24 hour mode
	
	i2c_start((RTC_ADDR<<1) | I2C_WRITE);
	i2c_write(0x00);
	i2c_stop();
	
	i2c_start((RTC_ADDR<<1) | I2C_READ);
	
	*sec = bcdtodec(i2c_read_ack());
	*min = bcdtodec(i2c_read_ack());
	*hour = bcdtodec(i2c_read_nack() & 0x3F);
	
	i2c_stop();
}

void rtc_get_time_12h(uint8_t *hour, uint8_t *min, uint8_t *sec, uint8_t *am_pm)
{
	// 12 hour mode
	
	uint8_t temphour;
	
	i2c_start((RTC_ADDR<<1) | I2C_WRITE);
	i2c_write(0x00);
	i2c_stop();
	
	i2c_start((RTC_ADDR<<1) | I2C_READ);
	
	*sec = bcdtodec(i2c_read_ack());
	*min = bcdtodec(i2c_read_ack());
	temphour = i2c_read_nack();
	
	i2c_stop();

	*hour =  bcdtodec(temphour & 0x1F);

	if (temphour & 0x20)	// if PM
	*am_pm = 1;
	else					// if AM
	*am_pm = 0;
}

void rtc_get_date(uint16_t *year, uint8_t *month, uint8_t *date)
{
	uint8_t tempmonth = 0, tempyear = 0;
	i2c_start((RTC_ADDR<<1) | I2C_WRITE);
	i2c_write(0x04);
	i2c_stop();
	
	i2c_start((RTC_ADDR<<1) | I2C_READ);
	
	*date = bcdtodec(i2c_read_ack());
	tempmonth = i2c_read_ack();
	tempyear = bcdtodec(i2c_read_nack());
	
	i2c_stop();
	
	*month = bcdtodec(tempmonth & 0x1F);
	if(tempmonth & 0x80)
		*year = tempyear + 2000;
	else 
		*year = tempyear + 1900;
}
uint8_t rtc_get_day()
{
	uint8_t day;
	i2c_start((RTC_ADDR<<1) | I2C_WRITE);
	i2c_write(0x03);
	i2c_stop();
	
	i2c_start((RTC_ADDR<<1) | I2C_READ);
	day = bcdtodec(i2c_read_nack() & 0x07);
	i2c_stop();
	
	return day;
}
