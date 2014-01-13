/*
 *	Real Time Clock driver for Wolfson Microelectronics tps65910
 *
 *	Copyright (C) 2009 Wolfson Microelectronics PLC.
 *
 *  Author: Mark Brown <broonie@opensource.wolfsonmicro.com>
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *  V1: TRS65910标准化整合,RTC和ALARM的修改,宏定义的包括,I2C写的中断保护.2013.12.26
 *  V2: 在原来的基础上面,把RTC的文件单独分离开来,修改了对3F寄存器最低位写限制.2013.12.31
 *  V3: 加入低电开机保护功能（寄存器0x1D=0x0D）.屏蔽对3F寄存器的bit 1的操作.2014.01.02
 *  V4: 去掉使用TRS65910的变量判断,保留信息输出.2014.01.06
 *  V5: 加入休眠待机的时候低电PMU自动关机功能,增加trs65910.c和trs65910-irq.c文件,修改makefile文件.2014.01.13
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/time.h>
#include <linux/rtc.h>
#include <linux/slab.h>
#include <linux/bcd.h>
#include <linux/interrupt.h>
#include <linux/ioctl.h>
#include <linux/completion.h>
#include <linux/mfd/tps65910.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>


/* RTC Definitions */
/* RTC_CTRL_REG bitfields */
#define BIT_RTC_CTRL_REG_STOP_RTC_M		0x01
#define BIT_RTC_CTRL_REG_ROUND_30S_M		0x02
#define BIT_RTC_CTRL_REG_AUTO_COMP_M		0x04
#define BIT_RTC_CTRL_REG_MODE_12_24_M		0x08
#define BIT_RTC_CTRL_REG_TEST_MODE_M		0x10
#define BIT_RTC_CTRL_REG_SET_32_COUNTER_M	0x20
#define BIT_RTC_CTRL_REG_GET_TIME_M		0x40
#define BIT_RTC_CTRL_REG_RTC_V_OPT_M		0x80

/* RTC_STATUS_REG bitfields */
#define BIT_RTC_STATUS_REG_RUN_M		0x02
#define BIT_RTC_STATUS_REG_1S_EVENT_M		0x04
#define BIT_RTC_STATUS_REG_1M_EVENT_M		0x08
#define BIT_RTC_STATUS_REG_1H_EVENT_M		0x10
#define BIT_RTC_STATUS_REG_1D_EVENT_M		0x20
#define BIT_RTC_STATUS_REG_ALARM_M		0x40
#define BIT_RTC_STATUS_REG_POWER_UP_M		0x80

/* RTC_INTERRUPTS_REG bitfields */
#define BIT_RTC_INTERRUPTS_REG_EVERY_M		0x03
#define BIT_RTC_INTERRUPTS_REG_IT_TIMER_M	0x04
#define BIT_RTC_INTERRUPTS_REG_IT_ALARM_M	0x08

/* DEVCTRL bitfields */
#define BIT_RTC_PWDN				0x40

/* REG_SECONDS_REG through REG_YEARS_REG is how many registers? */

#define ALL_ALM_REGS				6


#define RTC_SET_TIME_RETRIES	5
#define RTC_GET_TIME_RETRIES	5

static int trs65910_debug_level = 0;//DEBUG_INFO;
static u8 trs65910_version = 5;//FIX VERSION NUM(RANGE 1-16);
static u8 trs65910_read_version = 0;//FIX VERSION NUM(RANGE 1-16);
static ssize_t trs65910_version_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	ssize_t size = 0;

	if(NULL == buf)
	{
		return -1;
	}

	size = sprintf(buf, "set version num: %d, read version num: %d\n", trs65910_version, trs65910_read_version);

	return size;

}

static ssize_t trs65910_debug_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	ssize_t size = 0;

	if(NULL == buf)
	{
		return -1;
	}

	size = sprintf(buf, "%d\n", trs65910_debug_level);

	return size;

}

static ssize_t trs65910_debug_store(
		struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
	int val = 0;

	sscanf(buf, "%d", &val);

	trs65910_debug_level = val;

	return size;
}

static DEVICE_ATTR(version, 0664, trs65910_version_show, NULL);
static DEVICE_ATTR(pmu_debug, 0664, trs65910_debug_show, trs65910_debug_store);

struct tps65910_rtc {
	struct tps65910 *tps65910;
	struct rtc_device *rtc;
	unsigned int alarm_enabled:1;
};

/*
 * Read current time and date in RTC
 */
#define ALL_TIME_REGS_TRS					6//FOR TRS65910
#define ALL_TIME_BACK_REGS_TRS			5//FOR TRS65910

#define DBG(x...)	\
	do { \
		if (trs65910_debug_level > 0) { \
			printk(KERN_INFO x); \
		} \
	} while (0)

//计算某天星期几(Zeller)
static int countWeakOfDays( int year, int month, int days ) {
	int result = 0, century = 0;
	century = year / 100;
	year = year - century * 100;
	result = year + (year/4) + (century/4) - 2 * century + ((26 * (month + 1))/10) + (days - 1);
	if (result > 0) {
		result = result % 7;
	} else {
		result = (result + 70) % 7;
	}
	//DBG("******%s..The Wead of Days(Year:%d:Month:%d:Days:%d) is:%d!\n", __func__, 
	//	year, month, days, result);
	return result;
}

//一个整月的天数
static int daysOfMonth( int y, int m ){
	if(m==2){
		if(y%4==0&&y%100!=0||y%400==0){
			return 29;
		}else{
			return 28;
		}
		}else if(m==4||m==6||m==9||m==11){
			return 30;
		}else{
			return 31;
	}
}


 //判断输入有效
static int valiDate( int y, int m, int d ){
	if( y > 3000 || y < 1900 ){
		DBG("error year!\n");
		return 1;
	}
	if( m > 12 || m < 1 ){
		DBG("error month!\n");
		return 1;
	}
	if( d>daysOfMonth(y,m) || d<1 ){
		DBG("error day!\n");
		return 1;
	}
	return 0;
}

//输入年份已经过的天数
static int daysBefore( int y, int m, int d ){
	int i,result=0;
	for( i = 1; i <= 12; i++ ){
		if( i == m ){
			result += d;
			return result;
		}else{
			result += daysOfMonth(y, i);
		}
	}
}

//计算每年天数
static int daysOfYear( int y ){
	if( ((y%4==0) && (y%100!=0)) || (y%400==0)){
		return 366;
	}else 
		return 365;
}

//计算总天数
static int resultAllDays( int rtcYear, int rtcMonth, int rtcDays, 
	int setYear, int setMonth, int setDays ) {
	unsigned int result = 0;
	int i;
	if ( valiDate(rtcYear, rtcMonth, rtcDays) || valiDate(setYear, setMonth, setDays) )
		return -1;//if error,time set 1970,1,1 peter
	
	for( i = rtcYear; i < setYear; i++ ){
		result += daysOfYear(i);
	}
	//DBG("******%s..RTC:%d,%d,%d-SET:%d, %d, %d\n", __func__, rtcYear, rtcMonth, rtcDays, 
	//	setYear, setMonth, setDays);
	result = result - daysBefore(rtcYear, rtcMonth, rtcDays) + daysBefore(setYear, setMonth, setDays);
	//DBG("******%s..The All Day is:%d!\n", __func__, result);
	return result;
}

//计算以某日期加上天数得出某日期
static int countAllDaysToDate( int Gi_year, int Gi_month, int Gi_day, int After_day, int *After_year, 
int *After_month, int *After_days) {
	int year = 0, month = 0, days = 0;

	year = Gi_year;
	month = Gi_month;
	days = Gi_day + After_day;
	if(days  > 0)
	{
		while(days  > daysOfMonth(year, month))
		{
			days -= daysOfMonth(year, month);
			month++;
			if(month > 12)
			{
				month = 1;
				year++;
			}
		}
	}
	*After_year = year;
	*After_month = month;
	*After_days = days;
	return 0;
}

static int tps65910_rtc_readtime(struct device *dev, struct rtc_time *tm)
{
	DBG("@@@@%s..%d\n", __func__, __LINE__);
	struct tps65910_rtc *tps65910_rtc = dev_get_drvdata(dev);
	struct tps65910 *tps65910 = tps65910_rtc->tps65910;
	int ret;
	int count = 0;
	unsigned char rtc_data[ALL_TIME_REGS_TRS + 1];
	unsigned char back_save_rtc_data [5];
	struct rtc_time gb_back_save_val;
	u32 gb_back_save_reg32;
	u8 gb_back_save_reg8;
	int add = 0;
	u8 rtc_ctl;
	u8 dayOfMonth = 0;
	u8 currentmonth = 0;
	u32 totalDays = 0;
	
	memset(&gb_back_save_val, 0 , sizeof(struct rtc_time));

/****************Auto detect 65910**********for trs65910 Trsilicon peter**************/
	/*Dummy read*/	
	ret = tps65910_reg_read(tps65910, TPS65910_RTC_CTRL);
	
	/* Has the RTC been programmed? */
	ret = tps65910_reg_read(tps65910, TPS65910_RTC_CTRL);
	if (ret < 0) {
		dev_err(dev, "Failed to read RTC control: %d\n", ret);
		return ret;
	}
	rtc_ctl = ret & (~BIT_RTC_CTRL_REG_RTC_V_OPT_M);

	ret = tps65910_reg_write(tps65910, TPS65910_RTC_CTRL, rtc_ctl);
	if (ret < 0) {
		dev_err(dev, "Failed to write RTC control: %d\n", ret);
		return ret;
	}

	count = 5;
	do{
		ret = tps65910_bulk_read(tps65910, TPS65910_SECONDS,
				       ALL_TIME_REGS_TRS, &rtc_data[0]);
		if (ret != 0){
			if (count != 0)
				continue;
			else 
				goto ERROR_OUT;
		}else 
			break;

	}while(count--);

	DBG( "READ REG(0x00-0x05):RTC date/time %4d-%02d-%02d %02d:%02d:%02d-%s..%d\n",
			1900 + rtc_data[5], rtc_data[4], rtc_data[3], 
			rtc_data[2], rtc_data[1], rtc_data[0], __func__, __LINE__);
		
	tm->tm_sec = bcd2bin(rtc_data[0]);
	tm->tm_min = bcd2bin(rtc_data[1]);
	tm->tm_hour = bcd2bin(rtc_data[2]);
	tm->tm_mday = bcd2bin(rtc_data[3]);
	tm->tm_mon = bcd2bin(rtc_data[4]) ;
	tm->tm_year = bcd2bin(rtc_data[5]) ;	
	DBG( "TM: RTC date/time %4d-%02d-%02d %02d:%02d:%02d-%s..%d\n",
			tm->tm_year + 1900, tm->tm_mon, tm->tm_mday, 
			tm->tm_hour, tm->tm_min, tm->tm_sec, __func__, __LINE__);

	count = 5;
	do{
		ret = tps65910_bulk_read(tps65910, TPS65910_BCK1,
				       ALL_TIME_BACK_REGS_TRS, &back_save_rtc_data[0]);
		if (ret != 0){
			if (count != 0)
				continue;
			else 
				goto ERROR_OUT;
		}else 
			break;
	}while(count--);

	gb_back_save_reg32 |= back_save_rtc_data[0];
	gb_back_save_reg32 |= back_save_rtc_data[1]<<8;
	gb_back_save_reg32 |= back_save_rtc_data[2]<<16;
	gb_back_save_reg32 |= back_save_rtc_data[3]<<24;

	DBG("READ BACK VALUE: TOTALDAY-VERSION-HOUR-MINUTE-SECOND: %d, %d, %d, %d, %d==%s..%d\n", 
		(u32)(gb_back_save_reg32>>15) & 0X1FFFF, (u8)(gb_back_save_reg32>>12) & 0x03, (u8)(back_save_rtc_data[4] & 0x1F), 
		(u8)(gb_back_save_reg32>>6) & 0x3F, (u8)(gb_back_save_reg32) & 0x3F, __func__, __LINE__);
	
	gb_back_save_val.tm_sec = back_save_rtc_data[4]&(1<<5) ? -((u8)gb_back_save_reg32&0x3f) : ((u8)gb_back_save_reg32&0x3f);
	gb_back_save_val.tm_min = back_save_rtc_data[4]&(1<<6) ? -((u8)(gb_back_save_reg32>>6)&0x3f) : ((u8)(gb_back_save_reg32>>6)&0x3f);
	gb_back_save_val.tm_hour = back_save_rtc_data[4]&(1<<7) ? -((u8)(back_save_rtc_data[4]&0x1f)) : ((u8)(back_save_rtc_data[4]&0x1f));
	//gb_back_save_val.tm_wday = ((u8)(gb_back_save_reg32>>12)&0x03);
	trs65910_read_version = (u8)(((gb_back_save_reg32>>12)&0x03) + 1);
	
	totalDays = ((u32)(gb_back_save_reg32>>15)&0x1ffff);

	countAllDaysToDate( tm->tm_year + 1900, tm->tm_mon, tm->tm_mday,
		totalDays, &gb_back_save_val.tm_year, &gb_back_save_val.tm_mon, &gb_back_save_val.tm_mday);

	DBG( "POST gb_back_save_val:YEAR-MONTH-DATA H:M:S %4d-%02d-%02d %02d:%02d:%02d-%s..%d\n",
			gb_back_save_val.tm_year, gb_back_save_val.tm_mon, gb_back_save_val.tm_mday,
			gb_back_save_val.tm_hour, gb_back_save_val.tm_min, gb_back_save_val.tm_sec, __func__, __LINE__);

	//DBG( "tm->tm_sec:%d:gb_back_save_val.tm_sec:%d-%d\n", tm->tm_sec, gb_back_save_val.tm_sec, __LINE__);
	tm->tm_sec = gb_back_save_val.tm_sec + tm->tm_sec;
	add = 0;
	if (tm->tm_sec > 59)
	{
		tm->tm_sec -= 60;
		add = 1;
	}
	else if (tm->tm_sec < 0)
	{
		tm->tm_sec = 60 + tm->tm_sec;
		add = -1;
	}
	//DBG( "tm->tm_sec:%d:add:%d-%d\n", tm->tm_sec, add, __LINE__);
	//DBG( "tm->tm_min:%d:gb_back_save_val.tm_min:%d-%d\n", tm->tm_min, gb_back_save_val.tm_min, __LINE__);
	tm->tm_min = tm->tm_min + gb_back_save_val.tm_min + add;
	
	add = 0;
	if (tm->tm_min > 59)
	{
		tm->tm_min -= 60;
		add = 1;
	}
	else if (tm->tm_min < 0)
	{
		tm->tm_min = 60 + tm->tm_min;
		add = -1;
	}
	//DBG( "tm->tm_min:%d:add:%d-%d\n", tm->tm_min, add, __LINE__);
	//DBG( "tm->tm_hour:%d:gb_back_save_val.tm_hour:%d-%d\n", tm->tm_hour, gb_back_save_val.tm_hour, __LINE__);
	tm->tm_hour = tm->tm_hour+ gb_back_save_val.tm_hour + add;
	
	add = 0;
	if (tm->tm_hour > 23)
	{
		tm->tm_hour -= 24;
		add = 1;
	}
	else if (tm->tm_hour < 0)
	{
		tm->tm_hour = 24 + tm->tm_hour;
		add = -1;
	}
	//DBG( "tm->tm_hour:%d:add:%d-%d\n", tm->tm_hour, add, __LINE__);
	//DBG( "gb_back_save_val.tm_mday:%d-%d\n", gb_back_save_val.tm_mday, __LINE__);
	
	tm->tm_mday = gb_back_save_val.tm_mday + add;
	dayOfMonth = daysOfMonth(gb_back_save_val.tm_year, gb_back_save_val.tm_mon);
	
	add = 0;
	if (tm->tm_mday > dayOfMonth)
	{
		tm->tm_mday = tm->tm_mday - dayOfMonth;
		add = 1;
	}
	else if (tm->tm_mday < 1)
	{	
		if (gb_back_save_val.tm_mon <= 1)
			currentmonth = 12;
		else
			currentmonth = gb_back_save_val.tm_mon - 1;
		dayOfMonth = daysOfMonth(gb_back_save_val.tm_year, currentmonth);
		tm->tm_mday = dayOfMonth;
		add = -1;
	}
	//DBG( "tm->tm_mday:%d:add:%d-%d\n", tm->tm_mday, add, __LINE__);
	//DBG( "back_save_val.tm_mon:%d-%d\n", gb_back_save_val.tm_mon, __LINE__);
	tm->tm_mon = gb_back_save_val.tm_mon + add;

	add = 0;
	if (tm->tm_mon > 12)
	{
		tm->tm_mon -= 12;
		add = 1;
	}
	else if (tm->tm_mon < 1) {
		tm->tm_mon = 12;
		add = -1;
	}
	//DBG( "gb_back_save_val.tm_year:%d-%d\n", gb_back_save_val.tm_year, __LINE__);
	tm->tm_year = tm->tm_year + gb_back_save_val.tm_year + add;
	//DBG( "tm->tm_year:%d:add:%d-%d\n", tm->tm_year, add, __LINE__);
	gb_back_save_val.tm_wday = countWeakOfDays(tm->tm_year + 1900, tm->tm_mon, tm->tm_mday);
	tm->tm_wday = gb_back_save_val.tm_wday;

	tm->tm_mon -= 1;
	tm->tm_year -= 1900;
	DBG("POST END TM RTC date/time %4d-%02d-%02d %02d:%02d:%02d\n",
			tm->tm_year + 1900, tm->tm_mon+1, tm->tm_mday,
			tm->tm_hour, tm->tm_min, tm->tm_sec);

	return ret;

ERROR_OUT:
	dev_err(dev, "Timed out reading current time\n");

	return -EIO;
}

/*
 * Set current time and date in RTC
 */
static int tps65910_rtc_set_time(struct device *dev, struct rtc_time *tm)
{
	DBG("@@@@%s..%d\n", __func__, __LINE__);
	struct tps65910_rtc *tps65910_rtc = dev_get_drvdata(dev);
	struct tps65910 *tps65910 = tps65910_rtc->tps65910;
	int ret = 0;
	u8 rtc_ctl = 0;
	u8  regval = 0;
	u32  regval32 = 0;
	int count = 0;
	int add = 0;
	unsigned char rtc_data[ALL_TIME_REGS_TRS + 1];
	unsigned char back_save_rtc_data [6];
	unsigned char  reg_val[4];
	struct rtc_time back_save_val;
	unsigned char back_save_rtc_data_x [5];
	
	memset(&back_save_val, 0 , sizeof(struct rtc_time));
	
	struct rtc_time gb_back_save_val;
	u32 gb_back_save_reg32 = 0;
	u8 gb_back_save_reg8 = 0;
	memset(&gb_back_save_val, 0 , sizeof(struct rtc_time));
/***********************read reg 0x10 use dynamic registers***********************/
	/*Dummy read*/	
	ret = tps65910_reg_read(tps65910, TPS65910_RTC_CTRL);
	
	/* Stop RTC while updating the TC registers */
	ret = tps65910_reg_read(tps65910, TPS65910_RTC_CTRL);
	if (ret < 0) {
		dev_err(dev, "Failed to read RTC control: %d\n", ret);
		return ret;
	}
	
	rtc_ctl = ret & (~BIT_RTC_CTRL_REG_STOP_RTC_M);
	ret = tps65910_reg_write(tps65910, TPS65910_RTC_CTRL, rtc_ctl);
	if (ret < 0) {
		dev_err(dev, "Failed to write RTC control: %d\n", ret);
		return ret;
	}


/********************* read reg 0x00-0x05 *********************/
	do{
		ret = tps65910_bulk_read(tps65910, TPS65910_SECONDS,
				       ALL_TIME_REGS_TRS, &rtc_data[0]);
		if (ret != 0){
			continue;
	}else 
		break;

	} while (++count < RTC_GET_TIME_RETRIES);

	DBG( "BEFORE READ RTC REG(0x00-0x05):RTC date/time %4d-%02d-%02d %02d:%02d:%02d-%s..%d\n",
			1900 + rtc_data[5], rtc_data[4], rtc_data[3], 
			rtc_data[2], rtc_data[1], rtc_data[0], __func__, __LINE__);
	back_save_val.tm_sec = bcd2bin(rtc_data[0]);
	back_save_val.tm_min = bcd2bin(rtc_data[1]);
	back_save_val.tm_hour = bcd2bin(rtc_data[2]);
	back_save_val.tm_mday= bcd2bin(rtc_data[3]);
	back_save_val.tm_mon= bcd2bin(rtc_data[4]);
	back_save_val.tm_year= bcd2bin(rtc_data[5]);
	DBG( "POST READ RTC REG(0x00-0x05):RTC date/time %4d-%02d-%02d %02d:%02d:%02d-%s..%d\n",
			1900 + back_save_val.tm_year, back_save_val.tm_mon, back_save_val.tm_mday, 
			back_save_val.tm_hour, back_save_val.tm_min, back_save_val.tm_sec, __func__, __LINE__);
	DBG( "BEFORE SET TM REG):RTC date/time %4d-%02d-%02d %02d:%02d:%02d-%s..%d\n",
			tm->tm_year, tm->tm_mon,  tm->tm_mday, 
			tm->tm_hour, tm->tm_min, tm->tm_sec, __func__, __LINE__);
	tm->tm_mon += 1;
	DBG( "LAST SET TM REG):RTC date/time %4d-%02d-%02d %02d:%02d:%02d-%s..%d\n",
			tm->tm_year + 1900, tm->tm_mon,  tm->tm_mday, 
			tm->tm_hour, tm->tm_min, tm->tm_sec, __func__, __LINE__);

/********************* post back time to reg 0x17-0x1B *********************/

	back_save_val.tm_sec = tm->tm_sec - bcd2bin(rtc_data[0]);
	back_save_val.tm_min = tm->tm_min - bcd2bin(rtc_data[1]);
	back_save_val.tm_hour = tm->tm_hour - bcd2bin(rtc_data[2]);
	//back_save_val.tm_wday= countWeakOfDays(tm->tm_year + 1900, tm->tm_mon, tm->tm_mday);
	back_save_val.tm_mday= bcd2bin(rtc_data[3]);
	back_save_val.tm_mon= bcd2bin(rtc_data[4]);
	back_save_val.tm_year= bcd2bin(rtc_data[5]);

	DBG( "TM-DELTAT: H:M:S = %d:%d:%d-%s..%d\n",
			back_save_val.tm_hour,back_save_val.tm_min,back_save_val.tm_sec, __func__, __LINE__);

	gb_back_save_reg32 = 0;
	regval = back_save_val.tm_sec;
	if (back_save_val.tm_sec < 0)
	{
		gb_back_save_reg8 |= 1<<5;
		regval = -back_save_val.tm_sec;
	}
	gb_back_save_reg32 |= regval;


	regval = back_save_val.tm_min;
	if (back_save_val.tm_min < 0)
	{
		gb_back_save_reg8 |= 1<<6;
		regval = -back_save_val.tm_min;
	}
	gb_back_save_reg32 |= regval<<6;


	regval = back_save_val.tm_hour;
	if ( back_save_val.tm_hour < 0)
	{
		gb_back_save_reg8 |= 1<<7;
		regval = -back_save_val.tm_hour;
	}
	gb_back_save_reg8 |= (regval & 0x1F);


	//regval = back_save_val.tm_wday;
	regval = (u8)(trs65910_version - 1);//SAVE VERSION NUM FROM START 0
	gb_back_save_reg32 |= regval<<12;

	DBG("******%s..resultAllDays:%d,%d,%d-SET:%d, %d, %d\n", __func__, back_save_val.tm_year, 
		back_save_val.tm_mon, back_save_val.tm_mday, tm->tm_year + 1900, tm->tm_mon, tm->tm_mday);
	regval32 = resultAllDays(back_save_val.tm_year + 1900, back_save_val.tm_mon, back_save_val.tm_mday, 
		tm->tm_year + 1900, tm->tm_mon, tm->tm_mday);
	if (regval32 < 0) {//if is error,set 1970,1,1 and second set 0 peter
		regval32 = 25567;
		gb_back_save_reg8 = 0;
		gb_back_save_reg32 = 0;
		regval = 0;
		gb_back_save_reg8 |= 0<<5;
		gb_back_save_reg32 |= regval;//if year is error,set 1970,1,1 and second set 0 peter

		gb_back_save_reg8 |= 0<<6;
		gb_back_save_reg32 |= regval<<6;//if year is error,set 1970,1,1 and minute set 0 peter

		gb_back_save_reg8 |= 0<<7;
		gb_back_save_reg8 |= (regval & 0x1F);//if year is error,set 1970,1,1 and hour set 0 peter


		//regval = countWeakOfDays(1970, 1, 1);//count 1970,1,1 weak peter
		regval = (u8)(trs65910_version - 1);//SAVE VERSION NUM FROM START 0
		gb_back_save_reg32 |= regval<<12;
	}
	gb_back_save_reg32 |= regval32<<15;

	DBG("WRITE BACK VALUE: TOTALDAY-VERSION-HOUR-MINUTE-SECOND: %d, %d, %d, %d, %d==%s..%d\n", 
		(u32)(gb_back_save_reg32>>15) & 0X1FFFF, (u8)(gb_back_save_reg32>>12) & 0x03, (u8)(gb_back_save_reg8 & 0x1F), 
		(u8)(gb_back_save_reg32>>6) & 0x3F, (u8)(gb_back_save_reg32) & 0x3F, __func__, __LINE__);

/********************* save back time to reg 0x17-0x1B *********************/
	ret = tps65910_reg_write(tps65910, TPS65910_BCK1, (u8)gb_back_save_reg32);
	if (ret < 0) {
		dev_err(dev, "Failed to write RTC times: %d\n", ret);
		return ret;
	}
	ret = tps65910_reg_write(tps65910, TPS65910_BCK1 + 1, (u8)(gb_back_save_reg32>>8));
	if (ret < 0) {
		dev_err(dev, "Failed to write RTC times: %d\n", ret);
		return ret;
	}
	ret = tps65910_reg_write(tps65910, TPS65910_BCK1 + 2, (u8)(gb_back_save_reg32>>16));
	if (ret < 0) {
		dev_err(dev, "Failed to write RTC times: %d\n", ret);
		return ret;
	}
	ret = tps65910_reg_write(tps65910, TPS65910_BCK1 + 3, (u8)(gb_back_save_reg32>>24));
	if (ret < 0) {
		dev_err(dev, "Failed to write RTC times: %d\n", ret);
		return ret;
	}
	ret = tps65910_reg_write(tps65910, TPS65910_BCK1 + 4, gb_back_save_reg8);
	if (ret < 0) {
		dev_err(dev, "Failed to write RTC times: %d\n", ret);
		return ret;
	}
/********************* read back time to reg 0x17-0x1B *********************/
// read out data from backup register for debuging
#if 0 // we should write this segment off  in release version
	count = 5;
	do{
		
		ret = tps65910_bulk_read(tps65910, 0x17,
				       1, &back_save_rtc_data_x[0]);
		if (ret != 0)
			continue;
		ret = tps65910_bulk_read(tps65910, 0x18,
				       1, &back_save_rtc_data_x[1]);
		if (ret != 0)
			continue;
		
		ret = tps65910_bulk_read(tps65910, 0x19,
				       1, &back_save_rtc_data_x[2]);
		if (ret != 0)
			continue;
		
		ret = tps65910_bulk_read(tps65910, 0x1a,
				       1, &back_save_rtc_data_x[3]);
		if (ret != 0)
			continue;
		
		ret = tps65910_bulk_read(tps65910, 0x1b,
				       1, &back_save_rtc_data_x[4]);
		if (ret != 0)
			continue;
		else 
			break;

	}while(count--);

	DBG("TEST READ BACK REG(0x17-0x1B): %d, %d, %d, %d, %d==%s..%d\n", 
		back_save_rtc_data_x[0], back_save_rtc_data_x[1], back_save_rtc_data_x[2], 
		back_save_rtc_data_x[3], back_save_rtc_data_x[4], __func__, __LINE__);
#endif

/***********************set rtc running***********************/

	/*Dummy read*/	
	ret = tps65910_reg_read(tps65910, TPS65910_RTC_CTRL);
	
	/* Start RTC again */
	ret = tps65910_reg_read(tps65910, TPS65910_RTC_CTRL);
	if (ret < 0) {
		dev_err(dev, "Failed to read RTC control: %d\n", ret);
		return ret;
	}
	
	rtc_ctl = ret | BIT_RTC_CTRL_REG_STOP_RTC_M;

	ret = tps65910_reg_write(tps65910, TPS65910_RTC_CTRL, rtc_ctl);
	if (ret < 0) {
		dev_err(dev, "Failed to write RTC control: %d\n", ret);
		return ret;
	}

	return 0;
}

/*
 * Read alarm time and date in RTC
 */
static int tps65910_rtc_readalarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	struct tps65910_rtc *tps65910_rtc = dev_get_drvdata(dev);
	int ret;
	unsigned char alrm_data[ALL_ALM_REGS + 1];
	DBG("@@@@%s..%d\n", __func__, __LINE__);

	int count = 0;
	unsigned char back_save_rtc_data [5];
	struct rtc_time gb_back_save_val;
	u32 gb_back_save_reg32;
	u8 gb_back_save_reg8;
	int add = 0;
	u8 dayOfMonth = 0;
	u8 currentmonth = 0;
	u32 totalDays = 0;
	
	memset(&gb_back_save_val, 0 , sizeof(struct rtc_time));
	count = 5;
	do{
		ret = tps65910_bulk_read(tps65910_rtc->tps65910, TPS65910_ALARM_SECONDS,
				       ALL_ALM_REGS, &alrm_data[0]);
		if (ret != 0){
			if (count != 0)
				continue;
			else 
				goto ERROR_OUT;
		}else 
			break;
	}while(count--);
	
	DBG( "@@@@%s..Alarm Read Time: Y-M-D H:M:S %4d-%02d-%02d %02d:%02d:%02d\n", __func__,
			alrm_data[5] + 1900, alrm_data[4],  alrm_data[3], 
			alrm_data[2], alrm_data[1], alrm_data[0] );

	/* some of these fields may be wildcard/"match all" */
	alrm->time.tm_sec = bcd2bin(alrm_data[0]);
	alrm->time.tm_min = bcd2bin(alrm_data[1]);
	alrm->time.tm_hour = bcd2bin(alrm_data[2]);
	alrm->time.tm_mday = bcd2bin(alrm_data[3]);
	alrm->time.tm_mon = bcd2bin(alrm_data[4]);
	alrm->time.tm_year = bcd2bin(alrm_data[5]);
	
	DBG( "@@@@%s..Alarm Post Read Time: Y-M-D H:M:S %4d-%02d-%02d %02d:%02d:%02d\n", __func__,
			alrm_data[5] + 1900, alrm_data[4],  alrm_data[3], 
			alrm_data[2], alrm_data[1], alrm_data[0] );

	do{
		ret = tps65910_bulk_read(tps65910_rtc->tps65910, TPS65910_BCK1,
				       5, &back_save_rtc_data[0]);
		if (ret != 0){
			if (count != 0)
				continue;
			else 
				goto ERROR_OUT;
		}else 
			break;
	}while(count--);
		
	gb_back_save_reg32 |= back_save_rtc_data[0];
	gb_back_save_reg32 |= back_save_rtc_data[1]<<8;
	gb_back_save_reg32 |= back_save_rtc_data[2]<<16;
	gb_back_save_reg32 |= back_save_rtc_data[3]<<24;

	gb_back_save_val.tm_sec = back_save_rtc_data[4]&(1<<5) ? -((u8)gb_back_save_reg32&0x3f) : ((u8)gb_back_save_reg32&0x3f);
	gb_back_save_val.tm_min = back_save_rtc_data[4]&(1<<6) ? -((u8)(gb_back_save_reg32>>6)&0x3f) : ((u8)(gb_back_save_reg32>>6)&0x3f);
	gb_back_save_val.tm_hour = back_save_rtc_data[4]&(1<<7) ? -((u8)(back_save_rtc_data[4]&0x1f)) : ((u8)(back_save_rtc_data[4]&0x1f));
	//gb_back_save_val.tm_wday = ((u8)(gb_back_save_reg32>>12)&0x03);

	totalDays = ((u32)(gb_back_save_reg32>>15)&0x1ffff);

	countAllDaysToDate( alrm->time.tm_year + 1900, alrm->time.tm_mon, alrm->time.tm_mday,
		totalDays, &gb_back_save_val.tm_year, &gb_back_save_val.tm_mon, &gb_back_save_val.tm_mday);

	alrm->time.tm_sec = gb_back_save_val.tm_sec + alrm->time.tm_sec;
	add = 0;
	if (alrm->time.tm_sec > 59)
	{
		alrm->time.tm_sec -= 60;
		add = 1;
	}
	else if (alrm->time.tm_sec < 0)
	{
		alrm->time.tm_sec = 60 + alrm->time.tm_sec;
		add = -1;
	}
	alrm->time.tm_min = alrm->time.tm_min + gb_back_save_val.tm_min + add;
	
	add = 0;
	if (alrm->time.tm_min > 59)
	{
		alrm->time.tm_min -= 60;
		add = 1;
	}
	else if (alrm->time.tm_min < 0)
	{
		alrm->time.tm_min = 60 + alrm->time.tm_min;
		add = -1;
	}
	alrm->time.tm_hour = alrm->time.tm_hour+ gb_back_save_val.tm_hour + add;
	
	add = 0;
	if (alrm->time.tm_hour > 23)
	{
		alrm->time.tm_hour -= 24;
		add = 1;
	}
	else if (alrm->time.tm_hour < 0)
	{
		alrm->time.tm_hour = 24 + alrm->time.tm_hour;
		add = -1;
	}
	
	alrm->time.tm_mday = gb_back_save_val.tm_mday + add;
	dayOfMonth = daysOfMonth(gb_back_save_val.tm_year, gb_back_save_val.tm_mon);
	
	add = 0;
	if (alrm->time.tm_mday > dayOfMonth)
	{
		alrm->time.tm_mday = alrm->time.tm_mday - dayOfMonth;
		add = 1;
	}
	else if (alrm->time.tm_mday < 1)
	{	
		if (gb_back_save_val.tm_mon <= 1)
			currentmonth = 12;
		else
			currentmonth = gb_back_save_val.tm_mon - 1;
		dayOfMonth = daysOfMonth(gb_back_save_val.tm_year, currentmonth);
		alrm->time.tm_mday = dayOfMonth;
		add = -1;
	}
	alrm->time.tm_mon = gb_back_save_val.tm_mon + add;

	add = 0;
	if (alrm->time.tm_mon > 12)
	{
		alrm->time.tm_mon -= 12;
		add = 1;
	}
	else if (alrm->time.tm_mon < 1) {
		alrm->time.tm_mon = 12;
		add = -1;
	}
	alrm->time.tm_year = alrm->time.tm_year + gb_back_save_val.tm_year + add;
	gb_back_save_val.tm_wday = countWeakOfDays(alrm->time.tm_year + 1900, alrm->time.tm_mon, alrm->time.tm_mday);
	alrm->time.tm_wday = gb_back_save_val.tm_wday;
	
	alrm->time.tm_mon -= 1;
	alrm->time.tm_year -= 1900;
	DBG("POST END ALARM RTC date/time %4d-%02d-%02d %02d:%02d:%02d\n",
			alrm->time.tm_year + 1900, alrm->time.tm_mon+1, alrm->time.tm_mday,
			alrm->time.tm_hour, alrm->time.tm_min, alrm->time.tm_sec);
		
	ret = tps65910_reg_read(tps65910_rtc->tps65910, TPS65910_RTC_INTERRUPTS);
	if (ret < 0) {
		dev_err(dev, "Failed to read RTC control: %d\n", ret);
		return ret;
	}

	if (ret & BIT_RTC_INTERRUPTS_REG_IT_ALARM_M)
		alrm->enabled = 1;
	else
		alrm->enabled = 0;
	
	return 0;

ERROR_OUT:
	dev_err(dev, "Timed out reading current time\n");

	return -EIO;

}

static int tps65910_rtc_stop_alarm(struct tps65910_rtc *tps65910_rtc)
{
	tps65910_rtc->alarm_enabled = 0;

	return tps65910_clear_bits(tps65910_rtc->tps65910, TPS65910_RTC_INTERRUPTS,
			       BIT_RTC_INTERRUPTS_REG_IT_ALARM_M);

}

static int tps65910_rtc_start_alarm(struct tps65910_rtc *tps65910_rtc)
{
	tps65910_rtc->alarm_enabled = 1;

	return tps65910_set_bits(tps65910_rtc->tps65910, TPS65910_RTC_INTERRUPTS,
			       BIT_RTC_INTERRUPTS_REG_IT_ALARM_M);

}

static int tps65910_rtc_setalarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	struct tps65910_rtc *tps65910_rtc = dev_get_drvdata(dev);
	struct tps65910 *tps65910 = tps65910_rtc->tps65910;
	int ret;
	struct rtc_time tm;
	long now, scheduled, difftime = 0;
	int tmpsecond = 0, tmpmin = 0, tmphour = 0, tmpmday = 0;
	unsigned char alrm_data[ALL_TIME_REGS_TRS + 1];
	unsigned char rtc_data[ALL_TIME_REGS_TRS + 1];
	int add = 0, count = 0;
	int currentdayofmonth = 0;
	struct rtc_wkalrm tmpalarm;
	memset(&tmpalarm, 0 , sizeof(struct rtc_wkalrm));
	
	DBG( "@@@@%s..Alarm Set Time: Y-M-D H:M:S %4d-%02d-%02d %02d:%02d:%02d\n", __func__,
			alrm->time.tm_year + 1900, alrm->time.tm_mon + 1,  alrm->time.tm_mday, 
			alrm->time.tm_hour, alrm->time.tm_min, alrm->time.tm_sec );
	rtc_tm_to_time(&alrm->time, &scheduled);
	DBG( "@@@@scheduled: %ld\n", scheduled);
	tps65910_rtc_readtime(dev, &tm);
	DBG( "@@@@%s..Alarm Read Rtc Time: Y-M-D H:M:S %4d-%02d-%02d %02d:%02d:%02d\n", __func__,
			tm.tm_year + 1900, tm.tm_mon + 1,  tm.tm_mday, 
			tm.tm_hour, tm.tm_min, tm.tm_sec );
	rtc_tm_to_time(&tm, &now);
	DBG( "@@@@now: %ld\n", now);
	difftime = scheduled - now;
	DBG( "difftime: %ld\n", difftime);
	currentdayofmonth = daysOfMonth( tm.tm_year + 1900, tm.tm_mon);
	if ( difftime < 1 )
		goto ERROR_OUT;
	
	if ( difftime > 0) {
		tmpsecond = difftime % 60;
		difftime = difftime /60;
		if ( difftime > 0 ) {
			tmpmin = difftime % 60;
			difftime = difftime / 60;
			if ( difftime > 0 ) {
				tmphour = difftime % 24;
				difftime = difftime / 60;
				if ( difftime > 0 ) {
					tmpmday = difftime % currentdayofmonth;
				}
			}
		} 
	}
	DBG( "@@@@%s..Alarm Tmp DiffTime D:H:M:S %2d-%2d-%02d-%02d\n", __func__, 
			tmpmday, tmphour,  tmpmin, tmpsecond );
	
	ret = tps65910_rtc_stop_alarm(tps65910_rtc);
	if (ret < 0) {
		dev_err(dev, "Failed to stop alarm: %d\n", ret);
		goto ERROR_OUT;
	}

	count = 5;
	do{
		ret = tps65910_bulk_read(tps65910, TPS65910_SECONDS,
				       ALL_TIME_REGS_TRS, &rtc_data[0]);
		if (ret != 0){
			if (count != 0)
				continue;
			else 
				goto ERROR_OUT;
		}else 
			break;

	}while(count--);

		
	tm.tm_sec = bcd2bin(rtc_data[0]);
	tm.tm_min = bcd2bin(rtc_data[1]);
	tm.tm_hour = bcd2bin(rtc_data[2]);
	tm.tm_mday = bcd2bin(rtc_data[3]);
	tm.tm_mon = bcd2bin(rtc_data[4]) ;
	tm.tm_year = bcd2bin(rtc_data[5]) ;	
	
	DBG( "@@@@%s..Alarm Read Rtc Time: Y-M-D H:M:S %4d-%02d-%02d %02d:%02d:%02d\n", __func__,
			tm.tm_year + 1900, tm.tm_mon, tm.tm_mday, 
			tm.tm_hour, tm.tm_min, tm.tm_sec );
	
	if ( tmpsecond > 0 ) {
		tm.tm_sec += tmpsecond;
		if ( tm.tm_sec > 59 ) {
			tm.tm_sec -= 60;
			add = 1;
		}
	}

	if ( tmpmin > 0 || add > 0 ) {
		tm.tm_min = tm.tm_min + tmpmin + add;
		add = 0;
		if ( tm.tm_min > 59 ) {
			tm.tm_min -= 60;
			add = 1;
		}
	}

	if ( tmphour > 0 || add > 0) {
		tm.tm_hour = tm.tm_hour + tmphour + add;
		add = 0;
		if ( tm.tm_hour > 23 ) {
			tm.tm_hour -= 24;
			add = 1;
		}
	}
	
	if ( tmpmday> 0 || add > 0 ) {
		tm.tm_mday = tm.tm_mday + tmpmday + add;
		add = 0;
		if ( tm.tm_mday > currentdayofmonth ) {
			tm.tm_mday -= currentdayofmonth;
			add = 1;
			if ( add > 0 ) {
				tm.tm_mon = tm.tm_mon + add;
				add = 0;
				if ( tm.tm_mon > 12 ) {
					tm.tm_mon = 1;
					add = 1;
					if ( add > 0 ) {
						tm.tm_year += 1;
						add = 0;
					}
				}
			}
		}
	}
	DBG( "@@@@%s..Alarm Post TM: Y-M-D H:M:S %4d-%02d-%02d %02d:%02d:%02d\n", __func__,
			tm.tm_year + 1900, tm.tm_mon,  tm.tm_mday, 
			tm.tm_hour, tm.tm_min, tm.tm_sec );

	alrm_data[0] = bin2bcd(tm.tm_sec);
	alrm_data[1] = bin2bcd(tm.tm_min);
	alrm_data[2] = bin2bcd(tm.tm_hour);
	alrm_data[3] = bin2bcd(tm.tm_mday);
	alrm_data[4] = bin2bcd(tm.tm_mon);
	alrm_data[5] = bin2bcd(tm.tm_year);
	
	DBG( "@@@@%s..Alarm Write: Y-M-D H:M:S %4d-%02d-%02d %02d:%02d:%02d\n", __func__,
			alrm_data[5] + 1900, alrm_data[4],  alrm_data[3], 
			alrm_data[2], alrm_data[1], alrm_data[0] );
/********************* save back time to reg 0x08-0x0D *********************/
	count = 5;
	do{
		ret = tps65910_bulk_write(tps65910, TPS65910_ALARM_SECONDS,
				       ALL_ALM_REGS, &alrm_data[0]);
		if (ret != 0){
			if (count != 0)
				continue;
			else 
				goto ERROR_OUT;
		}else 
			break;

	}while(count--);


	if (ret != 0) {
		dev_err(dev, "Failed to read alarm time: %d\n", ret);
		goto ERROR_OUT;
	}

	if (alrm->enabled) {
		ret = tps65910_rtc_start_alarm(tps65910_rtc);
		if (ret < 0) {
			dev_err(dev, "Failed to start alarm: %d\n", ret);
			goto ERROR_OUT;
		}
	}

	return 0;

ERROR_OUT:
	dev_err(dev, "Timed out reading current time\n");
	return -EIO;
}

static int tps65910_rtc_alarm_irq_enable(struct device *dev,
				       unsigned int enabled)
{
	struct tps65910_rtc *tps65910_rtc = dev_get_drvdata(dev);

	if (enabled)
		return tps65910_rtc_start_alarm(tps65910_rtc);
	else
		return tps65910_rtc_stop_alarm(tps65910_rtc);
}

static int tps65910_rtc_update_irq_enable(struct device *dev,
				       unsigned int enabled)
{
	struct tps65910_rtc *tps65910_rtc = dev_get_drvdata(dev);

	if (enabled)
		return tps65910_set_bits(tps65910_rtc->tps65910, TPS65910_RTC_INTERRUPTS,
			       BIT_RTC_INTERRUPTS_REG_IT_TIMER_M);
	else
		return tps65910_clear_bits(tps65910_rtc->tps65910, TPS65910_RTC_INTERRUPTS,
			       BIT_RTC_INTERRUPTS_REG_IT_TIMER_M);
}

/*
 * We will just handle setting the frequency and make use the framework for
 * reading the periodic interupts.
 *
 * @freq: Current periodic IRQ freq:
 * bit 0: every second
 * bit 1: every minute
 * bit 2: every hour
 * bit 3: every day
 */
static int tps65910_rtc_irq_set_freq(struct device *dev, int freq)
{	
	struct tps65910_rtc *tps65910_rtc = dev_get_drvdata(dev);
	int ret;	
	u8 rtc_ctl;	
	
	if (freq < 0 || freq > 3)
		return -EINVAL;

	ret = tps65910_reg_read(tps65910_rtc->tps65910, TPS65910_RTC_INTERRUPTS);
	if (ret < 0) {
		dev_err(dev, "Failed to read RTC interrupt: %d\n", ret);
		return ret;
	}
	
	rtc_ctl = ret | freq;
	
	ret = tps65910_reg_write(tps65910_rtc->tps65910, TPS65910_RTC_INTERRUPTS, rtc_ctl);
	if (ret < 0) {
		dev_err(dev, "Failed to write RTC control: %d\n", ret);
		return ret;
	}
	
	return ret;
}

static irqreturn_t tps65910_alm_irq(int irq, void *data)
{
	struct tps65910_rtc *tps65910_rtc = data;
	int ret;
	u8 rtc_ctl;

	/*Dummy read -- mandatory for status register*/
	ret = tps65910_reg_read(tps65910_rtc->tps65910, TPS65910_RTC_STATUS);
	if (ret < 0) {
		DBG("%s:Failed to read RTC status: %d\n", __func__, ret);
		return ret;
	}
		
	ret = tps65910_reg_read(tps65910_rtc->tps65910, TPS65910_RTC_STATUS);
	if (ret < 0) {
		DBG("%s:Failed to read RTC status: %d\n", __func__, ret);
		return ret;
	}
	rtc_ctl = ret&0xff;

	//The alarm interrupt keeps its low level, until the micro-controller write 1 in the ALARM bit of the RTC_STATUS_REG register.	
	ret = tps65910_reg_write(tps65910_rtc->tps65910, TPS65910_RTC_STATUS,rtc_ctl);
	if (ret < 0) {
		DBG("%s:Failed to read RTC status: %d\n", __func__, ret);
		return ret;
	}
	
	rtc_update_irq(tps65910_rtc->rtc, 1, RTC_IRQF | RTC_AF);
	
	DBG("%s:irq=%d,rtc_ctl=0x%x\n",__func__,irq,rtc_ctl);
	return IRQ_HANDLED;
}

static irqreturn_t tps65910_per_irq(int irq, void *data)
{
	struct tps65910_rtc *tps65910_rtc = data;
	
	rtc_update_irq(tps65910_rtc->rtc, 1, RTC_IRQF | RTC_UF);

	//printk("%s:irq=%d\n",__func__,irq);
	return IRQ_HANDLED;
}
 
//low power shutdown
static irqreturn_t tps65910_vmbdch_irq(int irq, void *data)
{
	return IRQ_HANDLED;
}

static const struct rtc_class_ops tps65910_rtc_ops = {
	.read_time = tps65910_rtc_readtime,
	//.set_mmss = tps65910_rtc_set_mmss,
	.set_time = tps65910_rtc_set_time,
	.read_alarm = tps65910_rtc_readalarm,
	.set_alarm = tps65910_rtc_setalarm,
	.alarm_irq_enable = tps65910_rtc_alarm_irq_enable,
	//.update_irq_enable = tps65910_rtc_update_irq_enable,
	//.irq_set_freq = tps65910_rtc_irq_set_freq,
};

#ifdef CONFIG_PM
/* Turn off the alarm if it should not be a wake source. */
static int tps65910_rtc_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct tps65910_rtc *tps65910_rtc = dev_get_drvdata(&pdev->dev);
	int ret;
	
	if (tps65910_rtc->alarm_enabled && device_may_wakeup(&pdev->dev))
		ret = tps65910_rtc_start_alarm(tps65910_rtc);
	else
		ret = tps65910_rtc_stop_alarm(tps65910_rtc);

	if (ret != 0)
		dev_err(&pdev->dev, "Failed to update RTC alarm: %d\n", ret);

	return 0;
}

/* Enable the alarm if it should be enabled (in case it was disabled to
 * prevent use as a wake source).
 */
static int tps65910_rtc_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct tps65910_rtc *tps65910_rtc = dev_get_drvdata(&pdev->dev);
	int ret;

	if (tps65910_rtc->alarm_enabled) {
		ret = tps65910_rtc_start_alarm(tps65910_rtc);
		if (ret != 0)
			dev_err(&pdev->dev,
				"Failed to restart RTC alarm: %d\n", ret);
	}

	return 0;
}

/* Unconditionally disable the alarm */
static int tps65910_rtc_freeze(struct device *dev)
{
	DBG("@@@@%s..%d\n", __func__, __LINE__);
	struct platform_device *pdev = to_platform_device(dev);
	struct tps65910_rtc *tps65910_rtc = dev_get_drvdata(&pdev->dev);
	int ret;
	
	ret = tps65910_rtc_stop_alarm(tps65910_rtc);
	if (ret != 0)
		dev_err(&pdev->dev, "Failed to stop RTC alarm: %d\n", ret);

	return 0;
}
#else
#define tps65910_rtc_suspend NULL
#define tps65910_rtc_resume NULL
#define tps65910_rtc_freeze NULL
#endif

struct platform_device *g_pdev;
static int tps65910_rtc_probe(struct platform_device *pdev)
{
	struct tps65910 *tps65910 = dev_get_drvdata(pdev->dev.parent);
	struct tps65910_rtc *tps65910_rtc;
	int per_irq;
	int alm_irq;
	int ret = 0;
	u8 rtc_ctl;
	//low power shutdown
	int vmbdch_irq;
	
	struct rtc_time tm;
	struct rtc_time tm_def = {	//	2012.1.1 12:00:00 Saturday
			.tm_wday = 6,
			.tm_year = 111,
			.tm_mon = 0,
			.tm_mday = 1,
			.tm_hour = 12,
			.tm_min = 0,
			.tm_sec = 0,
		};
	
	tps65910_rtc = kzalloc(sizeof(*tps65910_rtc), GFP_KERNEL);
	if (tps65910_rtc == NULL)
		return -ENOMEM;
	
	platform_set_drvdata(pdev, tps65910_rtc);
	tps65910_rtc->tps65910 = tps65910;
	per_irq = tps65910->irq_base + TPS65910_IRQ_RTC_PERIOD;
	alm_irq = tps65910->irq_base + TPS65910_IRQ_RTC_ALARM;
	//low power shutdown
	vmbdch_irq = tps65910->irq_base + TPS65910_IRQ_VBAT_VMBDCH;
	
	/* Take rtc out of reset */
	ret = tps65910_reg_read(tps65910, TPS65910_DEVCTRL);
	if (ret < 0) {
		dev_err(&pdev->dev, "Failed to read TPS65910_DEVCTRL: %d\n", ret);
		return ret;
	}

	if(ret & BIT_RTC_PWDN)
	{
		rtc_ctl = ret & (~BIT_RTC_PWDN);

		ret = tps65910_reg_write(tps65910, TPS65910_DEVCTRL, rtc_ctl);
		if (ret < 0) {
			dev_err(&pdev->dev, "Failed to write RTC control: %d\n", ret);
			return ret;
		}
	}
	
	/*start rtc default*/
	ret = tps65910_reg_read(tps65910, TPS65910_RTC_CTRL);
	if (ret < 0) {
		dev_err(&pdev->dev, "Failed to read RTC control: %d\n", ret);
		return ret;
	}

	if(!(ret & BIT_RTC_CTRL_REG_STOP_RTC_M))
	{
		rtc_ctl = ret | BIT_RTC_CTRL_REG_STOP_RTC_M;

		ret = tps65910_reg_write(tps65910, TPS65910_RTC_CTRL, rtc_ctl);
		if (ret < 0) {
			dev_err(&pdev->dev, "Failed to write RTC control: %d\n", ret);
			return ret;
		}
	}
	
	ret = tps65910_reg_read(tps65910, TPS65910_RTC_STATUS);
	if (ret < 0) {
		dev_err(&pdev->dev, "Failed to read RTC status: %d\n", ret);
		return ret;
	}
	
	/*set init time*/
	ret = tps65910_rtc_readtime(&pdev->dev, &tm);
	if (ret)
	{
		dev_err(&pdev->dev, "Failed to read RTC time\n");
		return ret;
	}
	
	ret = rtc_valid_tm(&tm);
	if (ret) {
		dev_err(&pdev->dev,"invalid date/time and init time\n");
		tps65910_rtc_set_time(&pdev->dev, &tm_def); // 2011-01-01 12:00:00
		dev_info(&pdev->dev, "set RTC date/time %4d-%02d-%02d(%d) %02d:%02d:%02d\n",
				1900 + tm_def.tm_year, tm_def.tm_mon + 1, tm_def.tm_mday, tm_def.tm_wday,
				tm_def.tm_hour, tm_def.tm_min, tm_def.tm_sec);
	}

	device_init_wakeup(&pdev->dev, 1);

	tps65910_rtc->rtc = rtc_device_register("tps65910", &pdev->dev,
					      &tps65910_rtc_ops, THIS_MODULE);
	if (IS_ERR(tps65910_rtc->rtc)) {
		ret = PTR_ERR(tps65910_rtc->rtc);
		goto err;
	}

	/*request rtc and alarm irq of tps65910*/
	ret = request_threaded_irq(per_irq, NULL, tps65910_per_irq,
				   IRQF_TRIGGER_RISING, "RTC period",
				   tps65910_rtc);
	if (ret != 0) {
		dev_err(&pdev->dev, "Failed to request periodic IRQ %d: %d\n",
			per_irq, ret);
	}

	ret = request_threaded_irq(alm_irq, NULL, tps65910_alm_irq,
				   IRQF_TRIGGER_RISING, "RTC alarm",
				   tps65910_rtc);
	if (ret != 0) {
		dev_err(&pdev->dev, "Failed to request alarm IRQ %d: %d\n",
			alm_irq, ret);
	}
	
	//low power shutdown
	ret = request_threaded_irq(vmbdch_irq, NULL, tps65910_vmbdch_irq,
								IRQF_TRIGGER_FALLING, "VMBDCH",
								0);
	if (ret != 0) {
		dev_err(&pdev->dev, "Failed to request vmbdch_irq IRQ %d: %d\n",
				vmbdch_irq, ret);
	}
	//for rtc irq test
	//tps65910_set_bits(tps65910_rtc->tps65910, TPS65910_RTC_INTERRUPTS,
	//			       BIT_RTC_INTERRUPTS_REG_IT_TIMER_M);

	enable_irq_wake(alm_irq); // so tps65910 alarm irq can wake up system
	//low power shutdown
	enable_irq_wake(vmbdch_irq); // so tps65910 vmbdch irq can wake up system
 	g_pdev = pdev;
	
	printk("%s:ok\n",__func__);
	ret = device_create_file(tps65910->dev, &dev_attr_version);
	if (ret) {
		device_remove_file(tps65910->dev, &dev_attr_version);
		printk(KERN_ERR "Unable to create trs65910 dev file version\n");
	}
	ret = device_create_file(tps65910->dev, &dev_attr_pmu_debug);
	if (ret) {
		device_remove_file(tps65910->dev, &dev_attr_pmu_debug);
		printk(KERN_ERR "Unable to create trs65910 dev file pmu_debug\n");
	}
	
	return 0;

err:
	kfree(tps65910_rtc);
	return ret;
}

static int __devexit tps65910_rtc_remove(struct platform_device *pdev)
{
	struct tps65910_rtc *tps65910_rtc = platform_get_drvdata(pdev);
	int per_irq = tps65910_rtc->tps65910->irq_base + TPS65910_IRQ_RTC_PERIOD;
	int alm_irq = tps65910_rtc->tps65910->irq_base + TPS65910_IRQ_RTC_ALARM;

	free_irq(alm_irq, tps65910_rtc);
	free_irq(per_irq, tps65910_rtc);
	rtc_device_unregister(tps65910_rtc->rtc);
	kfree(tps65910_rtc);

	return 0;
}

static const struct dev_pm_ops tps65910_rtc_pm_ops = {
	.suspend = tps65910_rtc_suspend,
	.resume = tps65910_rtc_resume,

	.freeze = tps65910_rtc_freeze,
	.thaw = tps65910_rtc_resume,
	.restore = tps65910_rtc_resume,

	.poweroff = tps65910_rtc_suspend,
};

static struct platform_driver tps65910_rtc_driver = {
	.probe = tps65910_rtc_probe,
	.remove = __devexit_p(tps65910_rtc_remove),
	.driver = {
		.name = "tps65910-rtc",
		.pm = &tps65910_rtc_pm_ops,
	},
};

static ssize_t rtc_tps65910_test_write(struct file *file, 
			const char __user *buf, size_t count, loff_t *offset)
{
	char nr_buf[8];
	int nr = 0, ret;
	struct platform_device *pdev;	
	struct rtc_time tm;
	struct rtc_wkalrm alrm;
	struct tps65910_rtc *tps65910_rtc;
	
	if(count > 3)
		return -EFAULT;
	ret = copy_from_user(nr_buf, buf, count);
	if(ret < 0)
		return -EFAULT;

	sscanf(nr_buf, "%d", &nr);
	if(nr > 5 || nr < 0)
	{
		printk("%s:data is error\n",__func__);
		return -EFAULT;
	}

	if(!g_pdev)
		return -EFAULT;
	else
		pdev = g_pdev;

	
	tps65910_rtc = dev_get_drvdata(&pdev->dev);
	
	//test rtc time
	if(nr == 0)
	{	
		tm.tm_wday = 6;
		tm.tm_year = 111;
		tm.tm_mon = 0;
		tm.tm_mday = 1;
		tm.tm_hour = 12;
		tm.tm_min = 0;
		tm.tm_sec = 0;
	
		ret = tps65910_rtc_set_time(&pdev->dev, &tm); // 2011-01-01 12:00:00
		if (ret)
		{
			dev_err(&pdev->dev, "Failed to set RTC time\n");
			return -EFAULT;
		}

	}
	
	/*set init time*/
	ret = tps65910_rtc_readtime(&pdev->dev, &tm);
	if (ret)
		dev_err(&pdev->dev, "Failed to read RTC time\n");
	else
		dev_info(&pdev->dev, "RTC date/time %4d-%02d-%02d(%d) %02d:%02d:%02d\n",
			1900 + tm.tm_year, tm.tm_mon + 1, tm.tm_mday, tm.tm_wday,
			tm.tm_hour, tm.tm_min, tm.tm_sec);
		
	if(!ret)
	printk("%s:ok\n",__func__);
	else
	printk("%s:error\n",__func__);
	

	//test rtc alarm
	if(nr == 2)
	{
		//2000-01-01 00:00:30
		if(tm.tm_sec < 30)
		{
			alrm.time.tm_sec = tm.tm_sec+30;	
			alrm.time.tm_min = tm.tm_min;
		}
		else
		{
			alrm.time.tm_sec = tm.tm_sec-30;
			alrm.time.tm_min = tm.tm_min+1;
		}
		alrm.time.tm_hour = tm.tm_hour;
		alrm.time.tm_mday = tm.tm_mday;
		alrm.time.tm_mon = tm.tm_mon;
		alrm.time.tm_year = tm.tm_year;		
		tps65910_rtc_alarm_irq_enable(&pdev->dev, 1);
		tps65910_rtc_setalarm(&pdev->dev, &alrm);

		dev_info(&pdev->dev, "Set alarm %4d-%02d-%02d(%d) %02d:%02d:%02d\n",
				1900 + alrm.time.tm_year, alrm.time.tm_mon + 1, alrm.time.tm_mday, alrm.time.tm_wday,
				alrm.time.tm_hour, alrm.time.tm_min, alrm.time.tm_sec);
	}

	
	if(nr == 3)
	{	
		ret = tps65910_reg_read(tps65910_rtc->tps65910, TPS65910_RTC_STATUS);
		if (ret < 0) {
			printk("%s:Failed to read RTC status: %d\n", __func__, ret);
			return ret;
		}
		printk("%s:ret=0x%x\n",__func__,ret&0xff);

		ret = tps65910_reg_write(tps65910_rtc->tps65910, TPS65910_RTC_STATUS, ret&0xff);
		if (ret < 0) {
			printk("%s:Failed to read RTC status: %d\n", __func__, ret);
			return ret;
		}
	}

	if(nr == 4)
	tps65910_rtc_update_irq_enable(&pdev->dev, 1);

	if(nr == 5)
	tps65910_rtc_update_irq_enable(&pdev->dev, 0);
	
	return count;
}

static const struct file_operations rtc_tps65910_test_fops = {
	.write = rtc_tps65910_test_write,
};

static struct miscdevice rtc_tps65910_test_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "rtc_tps65910_test",
	.fops = &rtc_tps65910_test_fops,
};


static int __init tps65910_rtc_init(void)
{
	misc_register(&rtc_tps65910_test_misc);
	return platform_driver_register(&tps65910_rtc_driver);
}
subsys_initcall_sync(tps65910_rtc_init);

static void __exit tps65910_rtc_exit(void)
{	
        misc_deregister(&rtc_tps65910_test_misc);
	platform_driver_unregister(&tps65910_rtc_driver);
}
module_exit(tps65910_rtc_exit);

MODULE_DESCRIPTION("RTC driver for the tps65910 series PMICs");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:tps65910-rtc");
