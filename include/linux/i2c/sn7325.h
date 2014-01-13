#define SN7325_MAXGPIO		16
#define SN7325_BANK(offs)	((offs) >> 3)
#define SN7325_BIT(offs)	(1u << ((offs) & 0x7))

/* Register Defined */
#define INPUT_PORT_A  0x00
#define INPUT_PORT_B  0x01
#define OUTPUT_PORT_A 0x02
#define OUTPUT_PORT_B 0x03
#define CONFIG_PORT_A 0x04
#define CONFIG_PORT_B 0x05
#define INTCTL_PORT_A 0x06
#define INTCTL_PROT_B 0x07
#define SN7325_MAX_REG 0x08

struct sn7325_gpio_platform_data {
	int gpio_start;		/* GPIO Chip base # */
	unsigned irq_base;	/* interrupt base # */
};

