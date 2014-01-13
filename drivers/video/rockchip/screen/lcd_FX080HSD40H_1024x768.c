#ifndef __LCD_HDMI_1024x768__
#define __LCD_HDMI_1024x768__


/* Base */
#define SCREEN_TYPE	 	SCREEN_LVDS
#define LVDS_FORMAT     LVDS_8BIT_1

#define OUT_FACE		OUT_P888
#define DCLK			65000000
#define LCDC_ACLK       500000000//312000000//312000000           //29 lcdc axi DMA ÆµÂÊ

/* Timing */
#define H_PW			10
#define H_BP			100
#define H_VD			1024
#define H_FP			210

#define V_PW			10
#define V_BP			10
#define V_VD			768
#define V_FP			18

#define LCD_WIDTH       202
#define LCD_HEIGHT      152
/* Other */
#define DCLK_POL	0

#define DEN_POL		0
#define VSYNC_POL	0
#define HSYNC_POL	0

#define SWAP_RB		0
#define SWAP_RG		0
#define SWAP_GB		0  

#endif
