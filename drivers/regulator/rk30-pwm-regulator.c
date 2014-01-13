/*
 *
 * Copyright (C) 2013 ROCKCHIP, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/bug.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/clk.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/rk29-pwm-regulator.h>
#include <mach/iomux.h>
#include <linux/gpio.h>
#include <mach/board.h>
#include <plat/pwm.h>

#if 0
#define DBG(x...)	printk(KERN_INFO x)
#else
#define DBG(x...)
#endif

struct rk_pwm_dcdc {
        char name[16];
        struct regulator_desc desc;
        int pwm_id;
        struct clk *pwm_clk;
        const void __iomem *pwm_base;
        u32 suspend_hrc;
        u32 suspend_lrc;
        u32 backup_hrc;
        u32 backup_lrc;
        struct regulator_dev *regulator;
	struct pwm_platform_data *pdata;
};

#if defined(CONFIG_SOC_RK3168) || defined(CONFIG_SOC_RK3168M) || defined(CONFIG_ARCH_RK3188) || defined(CONFIG_ARCH_RK3026)
const static int pwm_voltage_map[] = {
	800000,825000,850000, 875000,900000, 925000 ,950000, 975000,1000000, 1025000, 1050000, 1075000, 1100000, 1125000, 1150000, 1175000, 1200000, 1225000, 1250000, 1275000, 1300000, 1325000, 1350000,1375000,1400000
};
#else
const static int pwm_voltage_map[] = {
	950000, 975000,1000000, 1025000, 1050000, 1075000, 1100000, 1125000, 1150000, 1175000, 1200000, 1225000, 1250000, 1275000, 1300000, 1325000, 1350000, 1375000, 1400000
};
#endif

static struct rk_pwm_dcdc *g_dcdc[2];

static int pwm_set_rate(struct rk_pwm_dcdc *dcdc,int nHz,u32 rate)
{
	u32 lrc, hrc;
	struct pwm_platform_data *pdata = dcdc->pdata;
	int id = pdata->pwm_id;
	unsigned long clkrate =0;

	clkrate = clk_get_rate(dcdc->pwm_clk);

	DBG("%s  rate=%d\n",__func__,rate);

	if(rate == 0)
	{
		// iomux pwm to gpio
		rk29_mux_api_set(pdata->pwm_iomux_name, pdata->pwm_iomux_gpio);
		//disable pull up or down
		gpio_pull_updown(pdata->pwm_gpio,PullDisable);
		// set gpio to low level
		gpio_direction_output(pdata->pwm_gpio,GPIO_LOW);
	}
	else if (rate < 100)
	{
		lrc = clkrate / nHz;
		lrc = lrc >> (1+(PWM_DIV>>9));
		lrc = lrc ? lrc : 1;
		hrc = lrc * rate / 100;
		hrc = hrc ? hrc : 1;

		// iomux pwm
		rk29_mux_api_set(pdata->pwm_iomux_name, pdata->pwm_iomux_pwm);

		rk_pwm_setup(id, PWM_DIV, hrc, lrc);
	}
	else if (rate == 100)
	{
		// iomux pwm to gpio
		rk29_mux_api_set(pdata->pwm_iomux_name, pdata->pwm_iomux_gpio);
		//disable pull up or down
		gpio_pull_updown(pdata->pwm_gpio,PullDisable);
		// set gpio to low level
		gpio_direction_output(pdata->pwm_gpio,GPIO_HIGH);

	}
	else
	{
		printk("%s:rate error\n",__func__);
		return -1;
	}

	usleep_range(10*1000, 10*1000);

	return (0);
}

static int pwm_regulator_list_voltage(struct regulator_dev *dev,unsigned int index)
{
	struct rk_pwm_dcdc *dcdc = rdev_get_drvdata(dev);
	if (index < dcdc->desc.n_voltages)
	return dcdc->pdata->pwm_voltage_map[index];
	else
		return -1;
}

static int pwm_regulator_is_enabled(struct regulator_dev *dev)
{
	DBG("Enter %s\n",__FUNCTION__);
	return 0;
}

static int pwm_regulator_enable(struct regulator_dev *dev)
{
	DBG("Enter %s\n",__FUNCTION__);
	return 0;
}

static int pwm_regulator_disable(struct regulator_dev *dev)
{
	DBG("Enter %s\n",__FUNCTION__);
	return 0;
}

static int pwm_regulator_get_voltage(struct regulator_dev *dev)
{
	//struct pwm_platform_data *pdata = rdev_get_drvdata(dev);
	
	struct rk_pwm_dcdc *dcdc = rdev_get_drvdata(dev);

	DBG("Enter %s\n",__FUNCTION__);  

	return (dcdc->pdata->pwm_voltage);
}

struct mutex mutex_pwm;

static int pwm_regulator_set_voltage(struct regulator_dev *dev,
		int min_uV, int max_uV, unsigned *selector)
{	   
	struct rk_pwm_dcdc *dcdc = rdev_get_drvdata(dev);
	const int *voltage_map = dcdc->pdata->pwm_voltage_map;
	int max = dcdc->pdata->max_uV;
	int coefficient = 0;

	u32 size = dcdc->desc.n_voltages, i, vol,pwm_value;

	DBG("%s:  min_uV = %d, max_uV = %d\n",__FUNCTION__, min_uV,max_uV);

	if (min_uV < voltage_map[0] ||max_uV > voltage_map[size-1])
	{
		printk("%s:voltage is out of table\n",__func__);
		return -EINVAL;
	}

	for (i = 0; i < size; i++)
	{
		if (voltage_map[i] >= min_uV)
			break;
	}

	mutex_lock(&mutex_pwm);
	coefficient = dcdc->pdata->coefficient;
	mutex_unlock(&mutex_pwm);

	vol =  voltage_map[i];

	dcdc->pdata->pwm_voltage = vol;

	
	if (dcdc->pwm_id != PWM_NULL)
	{
		// VDD12 = 1.40 - 0.455*D , 其中D为PWM占空比, 
		pwm_value = (max-vol)/coefficient/10;  // pwm_value %, coefficient *1000
	
		if (pwm_set_rate(dcdc,1000*1000,pwm_value)!=0)
		{
			printk("%s:fail to set pwm rate,pwm_value=%d\n",__func__,pwm_value);
			return -1;
		
		}
	}
	*selector = i;
	
	DBG("%s:ok,vol=%d,pwm_value=%d\n",__FUNCTION__,vol,pwm_value);

	return 0;

}

static struct regulator_ops pwm_voltage_ops = {
	.list_voltage	= pwm_regulator_list_voltage,
	.set_voltage	=pwm_regulator_set_voltage,
	.get_voltage	= pwm_regulator_get_voltage,
	.enable		= pwm_regulator_enable,
	.disable	= pwm_regulator_disable,
	.is_enabled	= pwm_regulator_is_enabled,
};

static ssize_t pwm_param_write(struct device *dev,struct device_attribute *attr,const char *buf, size_t size)
{
	struct platform_device *pdev = container_of(dev,struct platform_device,dev);
	struct pwm_platform_data *pdata = pdev->dev.platform_data;
	struct rk_pwm_dcdc *dcdc = platform_get_drvdata(pdev);
	unsigned selector = 0;
	int ret = 0;
	
	mutex_lock(&mutex_pwm);
	ret = strict_strtoul(buf, 10, &(pdata->coefficient));
	mutex_unlock(&mutex_pwm);
	if (ret)
		return ret;

	ret = pwm_regulator_set_voltage(dcdc->regulator,dcdc->pdata->pwm_voltage,dcdc->pdata->pwm_voltage,&selector);
	if(ret)
	{
		printk("%s pdata->coefficient=%d\n",__func__,pdata->coefficient);
		return ret;
	}

	printk("%s pdata->coefficient=%d\n",__func__,pdata->coefficient);

	return size;
}
static ssize_t pwm_param_read(struct device *dev,struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = container_of(dev,struct platform_device,dev);
	struct pwm_platform_data *pdata = pdev->dev.platform_data;
	int coefficient = pdata->coefficient;

	return sprintf(buf, "%d\n", (unsigned)coefficient);	
}

DEVICE_ATTR(pwm_param, 0664, pwm_param_read,pwm_param_write);

static int __devinit pwm_regulator_probe(struct platform_device *pdev)
{
	struct pwm_platform_data *pdata = pdev->dev.platform_data;
	struct rk_pwm_dcdc *dcdc;
	int pwm_id  =  pdata->pwm_id;
	int id = pdev->id;
	int ret ;
    	char gpio_name[20];
	unsigned selector = 0;
	
	if (!pdata)
		return -ENODEV;

	if (!pdata->pwm_voltage)
		pdata->pwm_voltage = 1100000;	// default 1.1v

	if(!pdata->pwm_voltage_map)
		pdata->pwm_voltage_map = pwm_voltage_map;

	if(!pdata->max_uV)
		pdata->max_uV = 1400000;

	if(!pdata->min_uV)
		pdata->min_uV = 1000000;
	
	if(pdata->suspend_voltage < pdata->min_uV)
		pdata->suspend_voltage = pdata->min_uV;
	
	if(pdata->suspend_voltage > pdata->max_uV)	
		pdata->suspend_voltage = pdata->max_uV;
	
	dcdc = kzalloc(sizeof(struct rk_pwm_dcdc), GFP_KERNEL);
	if (dcdc == NULL) {
		dev_err(&pdev->dev, "Unable to allocate private data\n");
		return -ENOMEM;
	}

	snprintf(dcdc->name, sizeof(dcdc->name), "PWM_DCDC%d", id + 1);
	dcdc->desc.name = dcdc->name;
	dcdc->desc.id = id;
	dcdc->desc.type = REGULATOR_VOLTAGE;
	dcdc->desc.n_voltages = ARRAY_SIZE(pwm_voltage_map);
	dcdc->desc.ops = &pwm_voltage_ops;
	dcdc->desc.owner = THIS_MODULE;
	dcdc->pdata = pdata;
	dcdc->pwm_id = pwm_id;
	printk("%s:n_voltages=%d,dcdc->desc.name=%s,dcdc->desc.id=%d\ pwm_id=%d\n",__func__,dcdc->desc.n_voltages,dcdc->desc.name,
dcdc->desc.id,pwm_id);
	dcdc->regulator = regulator_register(&dcdc->desc, &pdev->dev,
					     pdata->init_data, dcdc);
	if (IS_ERR(dcdc->regulator)) {
		ret = PTR_ERR(dcdc->regulator);
		dev_err(&pdev->dev, "Failed to register PWM_DCDC%d: %d\n",
			id + 1, ret);
		goto err;
	}

	snprintf(gpio_name, sizeof(gpio_name), "PWM_DCDC%d", id + 1);

	if (pwm_id != PWM_NULL)
	{
		ret = gpio_request(pdata->pwm_gpio,gpio_name);
		if (ret) {
			dev_err(&pdev->dev,"failed to request pwm gpio\n");
			goto err_gpio;
		}

		dcdc->pwm_clk = rk_pwm_get_clk(pwm_id);
		dcdc->pwm_base = rk_pwm_get_base(pwm_id);
		if (IS_ERR(dcdc->pwm_clk)) {
			printk("pwm_clk get error %p\n", dcdc->pwm_clk);
			return -EINVAL;
		}
		clk_enable(dcdc->pwm_clk);
	}

	dcdc->suspend_lrc = 0x12;
	switch (pdata->suspend_voltage)
	{
	case 1000000:
	default:
		dcdc->suspend_hrc = 0x10;
		break;
	case 1050000:
		dcdc->suspend_hrc = 0x0e;
		break;
	case 1100000:
		dcdc->suspend_hrc = 0x0c;
		break;
	case 1150000:
		dcdc->suspend_hrc = 0x0a;
		break;
	}

	switch (pwm_id) {
	case 0:
		g_dcdc[0] = dcdc;
		printk("%s  dcdc0 =dcdc\n",__func__);
		break;
	case 1:	
		g_dcdc[1] = dcdc;
		printk("%s  dcdc1 =dcdc\n",__func__);
		break;
	case 0xff:
		printk("\n\n************************************************\n");
		printk("****%s PWM0: logic as same as arm\n",__func__);
		printk("************************************************\n\n");
		break;
	default:
		printk("%s pwm_id=%s is an error\n\n",__func__,pwm_id);
	}

	mutex_init(&mutex_pwm);
	platform_set_drvdata(pdev, dcdc);	
	printk(KERN_INFO "pwm_regulator.%d: driver initialized\n,dcdc->suspend_hrc=%x,dcdc->suspend_lrc=%x\n\n",id,dcdc->suspend_hrc,
dcdc->suspend_lrc);
	pwm_regulator_set_voltage(dcdc->regulator,pdata->pwm_voltage,pdata->pwm_voltage,&selector);

	ret = device_create_file(&pdev->dev,&dev_attr_pwm_param);
	if(ret){
		ret = -EINVAL;
		printk(KERN_ERR "failed to create pwm param file\n");
		goto err_gpio;
	}
	
	return 0;


err_gpio:
	gpio_free(pdata->pwm_gpio);
err:
	printk("%s:error\n",__func__);
	return ret;

}

void pwm_suspend_voltage(void)
{
	unsigned int i;
	
	for (i=0; i<2; i++)
	{
		struct rk_pwm_dcdc *dcdc = g_dcdc[i];
	if(!dcdc)
		return;
	
	dcdc->backup_hrc = readl_relaxed(dcdc->pwm_base + PWM_REG_HRC);
	dcdc->backup_lrc = readl_relaxed(dcdc->pwm_base + PWM_REG_LRC);

	__rk_pwm_setup(dcdc->pwm_base, PWM_DIV, dcdc->suspend_hrc, dcdc->suspend_lrc);
	}	
}

void pwm_resume_voltage(void)
{
	unsigned int i;

	for (i=0; i<2; i++)
	{
		struct rk_pwm_dcdc *dcdc = g_dcdc[i];
	
	if(!dcdc)
		return;
	__rk_pwm_setup(dcdc->pwm_base, PWM_DIV, dcdc->backup_hrc, dcdc->backup_lrc);
	}
}

static int __devexit pwm_regulator_remove(struct platform_device *pdev)
{
	struct pwm_platform_data *pdata = pdev->dev.platform_data;
	struct regulator_dev *rdev = platform_get_drvdata(pdev);

	regulator_unregister(rdev);
	gpio_free(pdata->pwm_gpio);

	return 0;
}

static struct platform_driver pwm_regulator_driver = {
	.driver = {
		.name = "pwm-voltage-regulator",
	},
	.remove = __devexit_p(pwm_regulator_remove),
};


static int __init pwm_regulator_module_init(void)
{
	return platform_driver_probe(&pwm_regulator_driver, pwm_regulator_probe);
}

static void __exit pwm_regulator_module_exit(void)
{
	platform_driver_unregister(&pwm_regulator_driver);
}

fs_initcall(pwm_regulator_module_init);
module_exit(pwm_regulator_module_exit);
MODULE_LICENSE("GPL");
