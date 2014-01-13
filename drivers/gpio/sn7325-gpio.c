#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/irq.h>

#include <linux/i2c/sn7325.h>

#define DEBUG_INFO  0x01
#define DEBUG_WARN  0x02
#define DEBUG_ERR   0x04

static int sn7325_debug_level = 0;//DEBUG_INFO;

#define SN7325_DBG(level, fmt, ...)					\
	do {							\
		if (sn7325_debug_level >= level)			\
			pr_info("sn7325 dbg: " fmt, ## __VA_ARGS__);	\
	} while (0)

#define DRV_NAME	"sn7325-gpio"

struct sn7325_gpio {
	struct i2c_client *client;
	struct gpio_chip gpio_chip;
	struct mutex lock;	/* protect cached dir, dat_out */
	/* protect serialized access to the interrupt controller bus */
	struct mutex irq_lock;
	unsigned gpio_start;
	unsigned irq_base;
	uint8_t dat_out[2];
	uint8_t dir[2];
	uint8_t int_lvl[2];
	uint8_t int_en[2];
	uint8_t irq_mask[2];
	uint8_t irq_wake[2];
};

#define SN7325_I2C_SPEED 300*1000

static struct i2c_client *sn7325_client;

static ssize_t sn7325_regs_show(struct device_driver *driver, char *buf)
{
	u8 value[SN7325_MAX_REG] = {0,};
	int ret, i;
	ssize_t size = 0;

	if(NULL == buf)
	{
		return -1;
	}

	for (i=0; i<SN7325_MAX_REG; i++) {
		ret = i2c_master_reg8_recv(sn7325_client, INPUT_PORT_A+i, &value[0], 1, SN7325_I2C_SPEED);
		if (ret < 0)
			return sprintf(buf, "%s", "sn7325 regs read err\n");

		size += sprintf(buf+size, "0x%2x, ", value[0]);
	}
	
	return size;

}


static ssize_t sn7325_regs_store(struct device_driver *dev,
				      const char *buf, size_t count)
{
	int ret = 0;
	int reg, value;

	ret = sscanf(buf, "%x %x",
			&reg, &value);
	printk("ret = %d, reg=0x%2x, value=0x%2x\n", ret, reg, value);
	if (ret < 2)
		return -EINVAL;

	ret = i2c_master_reg8_send(sn7325_client, (u8)reg, (u8*)(&value), 1, SN7325_I2C_SPEED);
	if (ret < 0)
		printk("sn7325 write reg err\n");

	return count;
}

static ssize_t sn7325_dbg_show(struct device_driver *driver, char *buf)
{
	ssize_t size = 0;

	if(NULL == buf)
	{
		return -1;
	}

	size = sprintf(buf, "%d\n", sn7325_debug_level);

	return size;

}

static ssize_t sn7325_dbg_store(struct device_driver *dev,
				      const char *buf, size_t count)
{
	int val = 0;

	sscanf(buf, "%d", &val);

	sn7325_debug_level = val;

	return count;
}

static DRIVER_ATTR(regs, 0664, sn7325_regs_show, sn7325_regs_store);
static DRIVER_ATTR(dbg, 0664, sn7325_dbg_show, sn7325_dbg_store);

static int sn7325_gpio_read(struct i2c_client *client, u8 reg)
{
	int ret = 0;
	uint8_t data = 0;

	ret = i2c_master_reg8_recv(client, reg, &data, 1, SN7325_I2C_SPEED);
	if (ret < 0) {
		dev_err(&client->dev, "Read Error\n");
		return ret;
	}

	return data;
}

static int sn7325_gpio_write(struct i2c_client *client, u8 reg, u8 val)
{
	uint8_t data = val;
	int ret = 0;

	ret = i2c_master_reg8_send(client, reg, &data, 1, SN7325_I2C_SPEED);
	if (ret < 0) {
		dev_err(&client->dev, "Write Error\n");
		return ret;
	}
	return 0;
}

static int sn7325_gpio_get_value(struct gpio_chip *chip, unsigned off)
{
	struct sn7325_gpio *dev =
	    container_of(chip, struct sn7325_gpio, gpio_chip);

	SN7325_DBG(DEBUG_INFO, "enter function %s\n", __func__);

	return !!(sn7325_gpio_read(dev->client,
		  INPUT_PORT_A + SN7325_BANK(off)) & SN7325_BIT(off));
}

static void sn7325_gpio_set_value(struct gpio_chip *chip,
				   unsigned off, int val)
{
	unsigned bank, bit;
	struct sn7325_gpio *dev =
	    container_of(chip, struct sn7325_gpio, gpio_chip);
	
	SN7325_DBG(DEBUG_INFO, "enter function %s\n", __func__);
	
	bank = SN7325_BANK(off);
	bit = SN7325_BIT(off);

	mutex_lock(&dev->lock);
	if (val)
		dev->dat_out[bank] |= bit;
	else
		dev->dat_out[bank] &= ~bit;

	SN7325_DBG(DEBUG_INFO, "write reg 0x%2x = 0x%2x\n", OUTPUT_PORT_A + bank, dev->dat_out[bank]);

	sn7325_gpio_write(dev->client, OUTPUT_PORT_A + bank,
			   dev->dat_out[bank]);
	mutex_unlock(&dev->lock);
}

static int sn7325_gpio_direction_input(struct gpio_chip *chip, unsigned off)
{
	int ret;
	unsigned bank;
	struct sn7325_gpio *dev =
	    container_of(chip, struct sn7325_gpio, gpio_chip);

	SN7325_DBG(DEBUG_INFO, "enter function %s\n", __func__);

	bank = SN7325_BANK(off);

	mutex_lock(&dev->lock);
	dev->dir[bank] |= SN7325_BIT(off);
	SN7325_DBG(DEBUG_INFO, "write reg 0x%2x = 0x%2x\n", OUTPUT_PORT_A + bank, dev->dir[bank]);
	ret = sn7325_gpio_write(dev->client, CONFIG_PORT_A + bank, dev->dir[bank]);
	mutex_unlock(&dev->lock);

	return ret;
}

static int sn7325_gpio_direction_output(struct gpio_chip *chip,
					 unsigned off, int val)
{
	int ret = 0;
	unsigned bank, bit;
	struct sn7325_gpio *dev =
	    container_of(chip, struct sn7325_gpio, gpio_chip);
	
	SN7325_DBG(DEBUG_INFO, "enter function %s, %d\n", __func__, off);

	bank = SN7325_BANK(off);
	bit = SN7325_BIT(off);

	mutex_lock(&dev->lock);
	dev->dir[bank] &= ~bit;

	if (val)
		dev->dat_out[bank] |= bit;
	else
		dev->dat_out[bank] &= ~bit;

	SN7325_DBG(DEBUG_INFO, "write reg 0x%2x = 0x%2x\n", OUTPUT_PORT_A + bank, dev->dat_out[bank]);
	ret |= sn7325_gpio_write(dev->client, OUTPUT_PORT_A + bank,
				 dev->dat_out[bank]);

	SN7325_DBG(DEBUG_INFO, "write reg 0x%2x = 0x%2x\n", CONFIG_PORT_A + bank, dev->dir[bank]);
	ret |= sn7325_gpio_write(dev->client, CONFIG_PORT_A + bank,
				 dev->dir[bank]);
	mutex_unlock(&dev->lock);

	return ret;
}

static int sn7325_gpio_to_irq(struct gpio_chip *chip, unsigned off)
{
	struct sn7325_gpio *dev =
		container_of(chip, struct sn7325_gpio, gpio_chip);
	
	SN7325_DBG(DEBUG_INFO, "enter function %s\n", __func__);

	return dev->irq_base + off;
}

static void sn7325_irq_bus_lock(struct irq_data *d)
{
	struct sn7325_gpio *dev = irq_data_get_irq_chip_data(d);
	
	SN7325_DBG(DEBUG_INFO, "enter function %s\n", __func__);

	mutex_lock(&dev->irq_lock);
}

 /*
  * genirq core code can issue chip->mask/unmask from atomic context.
  * This doesn't work for slow busses where an access needs to sleep.
  * bus_sync_unlock() is therefore called outside the atomic context,
  * syncs the current irq mask state with the slow external controller
  * and unlocks the bus.
  */

static void sn7325_irq_bus_sync_unlock(struct irq_data *d)
{
	struct sn7325_gpio *dev = irq_data_get_irq_chip_data(d);
	int i;

	SN7325_DBG(DEBUG_INFO, "enter function %s\n", __func__);

	for (i = 0; i < SN7325_BANK(SN7325_MAXGPIO); i++)
		if (dev->int_en[i] ^ dev->irq_mask[i]) {
			dev->int_en[i] = dev->irq_mask[i];
			sn7325_gpio_write(dev->client, INTCTL_PORT_A + i,
					   dev->int_en[i]);
		}

	mutex_unlock(&dev->irq_lock);
}

static void sn7325_irq_mask(struct irq_data *d)
{
	struct sn7325_gpio *dev = irq_data_get_irq_chip_data(d);
	unsigned gpio = d->irq - dev->irq_base;

	SN7325_DBG(DEBUG_INFO, "enter function %s\n", __func__);

	dev->irq_mask[SN7325_BANK(gpio)] |= SN7325_BIT(gpio);
}

static void sn7325_irq_unmask(struct irq_data *d)
{
	struct sn7325_gpio *dev = irq_data_get_irq_chip_data(d);
	unsigned gpio = d->irq - dev->irq_base;
	
	SN7325_DBG(DEBUG_INFO, "enter function %s\n", __func__);

	dev->irq_mask[SN7325_BANK(gpio)] &= ~SN7325_BIT(gpio);
}

static int sn7325_irq_set_type(struct irq_data *d, unsigned int type)
{
	struct sn7325_gpio *dev = irq_data_get_irq_chip_data(d);
	uint16_t gpio = d->irq - dev->irq_base;
	unsigned bank, bit;

	SN7325_DBG(DEBUG_INFO, "enter function %s\n", __func__);
	
#if 0
	if ((type & IRQ_TYPE_EDGE_BOTH)) {
		dev_err(&dev->client->dev, "irq %d: unsupported type %d\n",
			d->irq, type);
		return -EINVAL;
	}
#endif

	bank = SN7325_BANK(gpio);
	bit = SN7325_BIT(gpio);

	if (type & (IRQ_TYPE_LEVEL_HIGH | IRQ_TYPE_EDGE_RISING))
		dev->int_lvl[bank] |= bit;
	else if (type & (IRQ_TYPE_LEVEL_LOW | IRQ_TYPE_EDGE_FALLING))
		dev->int_lvl[bank] &= ~bit;
	else
		return -EINVAL;

	//sn7325_gpio_direction_input(&dev->gpio_chip, gpio);

	return 0;
}

static int sn7325_irq_set_wake(struct irq_data *d, unsigned int on)
{
	struct sn7325_gpio *dev = irq_data_get_irq_chip_data(d);
	uint16_t gpio = d->irq - dev->irq_base;
	unsigned bank, bit;

	SN7325_DBG(DEBUG_INFO, "enter function %s\n", __func__);
		
	bank = SN7325_BANK(gpio);
	bit = SN7325_BIT(gpio);

	if (on)
		dev->irq_wake[bank] |= bit;
	else
		dev->irq_wake[bank] &= ~bit;

	return 0;
}

static struct irq_chip sn7325_irq_chip = {
	.name			= "sn7325",
	.irq_mask		= sn7325_irq_mask,
	.irq_unmask		= sn7325_irq_unmask,
	.irq_bus_lock		= sn7325_irq_bus_lock,
	.irq_bus_sync_unlock	= sn7325_irq_bus_sync_unlock,
	.irq_set_type		= sn7325_irq_set_type,	
	.irq_set_wake	= sn7325_irq_set_wake,
};

static irqreturn_t sn7325_irq_handler(int irq, void *devid)
{
	struct sn7325_gpio *dev = devid;
	uint8_t bank, bit, pending, port;
	
	SN7325_DBG(DEBUG_INFO, "enter function %s\n", __func__);

	for (bank = 0; bank < SN7325_BANK(SN7325_MAXGPIO); bank++, bit = 0) {
		port = sn7325_gpio_read(dev->client, INPUT_PORT_A+bank);

		pending = ~(dev->int_lvl[bank] ^ port);
		pending &= ~(dev->irq_mask[bank]);

		while (pending) {
			if (pending & (1 << bit)) {
				//handle_nested_irq(dev->irq_base +
					//	  (bank << 3) + bit);
				generic_handle_irq(dev->irq_base +
						  (bank << 3) + bit);
				pending &= ~(1 << bit);

			}
			bit++;
		}
	}

	return IRQ_HANDLED;
}

static int sn7325_irq_setup(struct sn7325_gpio *dev)
{
	struct i2c_client *client = dev->client;
	struct sn7325_gpio_platform_data *pdata = client->dev.platform_data;
	unsigned gpio;
	int ret;

	SN7325_DBG(DEBUG_INFO, "enter function %s\n", __func__);

	dev->irq_base = pdata->irq_base;
	mutex_init(&dev->irq_lock);

	for (gpio = 0; gpio < dev->gpio_chip.ngpio; gpio++) {
		int irq = gpio + dev->irq_base;
		irq_set_chip_data(irq, dev);
		irq_set_chip_and_handler(irq, &sn7325_irq_chip,
					 handle_level_irq);
		//irq_set_nested_thread(irq, 1);
#ifdef CONFIG_ARM
		/*
		 * ARM needs us to explicitly flag the IRQ as VALID,
		 * once we do so, it will also set the noprobe.
		 */
		set_irq_flags(irq, IRQF_VALID);
#else
		irq_set_noprobe(irq);
#endif
	}

	ret = gpio_request(client->irq, "sn7325-irq");
	if (ret) {
		dev_err(&client->dev, "failed to request gpio %d\n",
			client->irq);
		goto out;
	}
	gpio_direction_input(client->irq);

	client->irq = gpio_to_irq(client->irq);

	ret = request_threaded_irq(client->irq,
				   NULL,
				   sn7325_irq_handler,
				   IRQF_TRIGGER_LOW | IRQF_ONESHOT,
				   dev_name(&client->dev), dev);
	if (ret) {
		dev_err(&client->dev, "failed to request irq %d\n",
			client->irq);
		goto out;
	}

	dev->gpio_chip.to_irq = sn7325_gpio_to_irq;

	return 0;

out:
	dev->irq_base = 0;
	return ret;
}

static void sn7325_irq_teardown(struct sn7325_gpio *dev)
{
	SN7325_DBG(DEBUG_INFO, "enter function %s\n", __func__);

	if (dev->irq_base)
		free_irq(dev->client->irq, dev);
}

static int __devinit sn7325_gpio_probe(struct i2c_client *client,
					const struct i2c_device_id *id)
{
	struct sn7325_gpio_platform_data *pdata = client->dev.platform_data;
	struct sn7325_gpio *dev;
	struct gpio_chip *gc;
	int ret, i;

	SN7325_DBG(DEBUG_INFO, "enter function %s\n", __func__);

	if (pdata == NULL) {
		dev_err(&client->dev, "missing platform data\n");
		return -ENODEV;
	}

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (dev == NULL) {
		dev_err(&client->dev, "failed to alloc memory\n");
		return -ENOMEM;
	}

	dev->client = client;

	gc = &dev->gpio_chip;
	gc->direction_input = sn7325_gpio_direction_input;
	gc->direction_output = sn7325_gpio_direction_output;
	gc->get = sn7325_gpio_get_value;
	gc->set = sn7325_gpio_set_value;
	//gc->can_sleep = 1;

	gc->base = pdata->gpio_start;
	gc->ngpio = SN7325_MAXGPIO;
	gc->label = client->name;
	gc->owner = THIS_MODULE;

	mutex_init(&dev->lock);

	for (i = 0, ret = 0; i < SN7325_BANK(SN7325_MAXGPIO); i++) {
		dev->dat_out[i] = sn7325_gpio_read(client, OUTPUT_PORT_A + i);
		dev->dir[i] = sn7325_gpio_read(client, CONFIG_PORT_A + i);
		//disable interrupt function
		dev->int_en[i] = 0xff;
		dev->irq_mask[i] = 0xff;
		dev->irq_wake[i] = 0x00;
		ret = sn7325_gpio_write(client, INTCTL_PORT_A + i, dev->int_en[i]);
		if (ret)
			goto err;
	}

	if (pdata->irq_base) {
		ret = sn7325_irq_setup(dev);
		if (ret)
			goto err;
	}

	ret = gpiochip_add(&dev->gpio_chip);
	if (ret) {
		goto err_irq;
	}

	dev_info(&client->dev, "gpios %d..%d (IRQ Base %d) on a %s Rev.\n",
			gc->base, gc->base + gc->ngpio - 1,
			pdata->irq_base, client->name);

	i2c_set_clientdata(client, dev);

	sn7325_client = client;

	return 0;

err_irq:
	sn7325_irq_teardown(dev);
err:
	kfree(dev);
	return ret;
}

static int __devexit sn7325_gpio_remove(struct i2c_client *client)
{
	struct sn7325_gpio *dev = i2c_get_clientdata(client);
	int ret;

	SN7325_DBG(DEBUG_INFO, "enter function %s\n", __func__);

	if (dev->irq_base)
		free_irq(dev->client->irq, dev);

	ret = gpiochip_remove(&dev->gpio_chip);
	if (ret) {
		dev_err(&client->dev, "gpiochip_remove failed %d\n", ret);
		return ret;
	}

	kfree(dev);
	return 0;
}

static int sn7325_gpio_suspend(struct i2c_client *client, pm_message_t mesg)
{
	struct sn7325_gpio *dev = (struct sn7325_gpio *)i2c_get_clientdata(client);

	SN7325_DBG(DEBUG_INFO, "enter function %s\n", __func__);

	if (dev->irq_wake)
		enable_irq_wake(dev->client->irq);

	return 0;
}
static int sn7325_gpio_resume(struct i2c_client *client)
{
	struct sn7325_gpio *dev = (struct sn7325_gpio *)i2c_get_clientdata(client);

	SN7325_DBG(DEBUG_INFO, "enter function %s\n", __func__);

	if (dev->irq_wake)
		disable_irq_wake(dev->client->irq);
	
	return 0;
}


static const struct i2c_device_id sn7325_gpio_id[] = {
	{DRV_NAME, 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, sn7325_gpio_id);

static struct i2c_driver sn7325_gpio_driver = {
	.driver = {
		   .name = DRV_NAME,
		   },
	.probe = sn7325_gpio_probe,
	.remove = __devexit_p(sn7325_gpio_remove),
	.suspend = sn7325_gpio_suspend,
	.resume = sn7325_gpio_resume,
	.id_table = sn7325_gpio_id,
};

static int __init sn7325_gpio_init(void)
{
	int ret;
	ret = i2c_add_driver(&sn7325_gpio_driver);
	if (ret)
		printk(KERN_ERR "Unable to register sn7325 gpio driver\n");

	ret = driver_create_file(&(sn7325_gpio_driver.driver), &driver_attr_regs);
	if (ret) {
		i2c_del_driver(&sn7325_gpio_driver);
		printk(KERN_ERR "Unable to create sn7325 gpio driver file regs\n");
	}
	
	ret = driver_create_file(&(sn7325_gpio_driver.driver), &driver_attr_dbg);
	if (ret) {
		driver_remove_file(&(sn7325_gpio_driver.driver), &driver_attr_regs);
		i2c_del_driver(&sn7325_gpio_driver);
		printk(KERN_ERR "Unable to create sn7325 gpio driver file dbg\n");
	}

	return ret;
}

subsys_initcall_sync(sn7325_gpio_init);

static void __exit sn7325_gpio_exit(void)
{
	i2c_del_driver(&sn7325_gpio_driver);
}

module_exit(sn7325_gpio_exit);

MODULE_AUTHOR("lyx <lyx@rock-chips.com>");
MODULE_DESCRIPTION("GPIO SN7325 Driver");
MODULE_LICENSE("GPL");
