
#include "generic_sensor.h"
/*
*      Driver Version Note
*v0.0.1: this driver is compatible with generic_sensor
*v0.0.3:
*        add sensor_focus_af_const_pause_usr_cb;
*/
static int version = KERNEL_VERSION(0,0,3);
module_param(version, int, S_IRUGO);



static int debug;
module_param(debug, int, S_IRUGO|S_IWUSR);

#define dprintk(level, fmt, arg...) do {			\
	if (debug >= level) 					\
	printk(KERN_WARNING fmt , ## arg); } while (0)

#define SENSOR_DG(format, ...) dprintk(1, format, ## __VA_ARGS__)
/* Sensor Driver Configuration Begin */
#define SENSOR_NAME RK29_CAM_SENSOR_SIV120D
#define SENSOR_V4L2_IDENT V4L2_IDENT_SIV120D
#define SENSOR_ID 0x12
#define SENSOR_BUS_PARAM					 (SOCAM_MASTER |\
											 SOCAM_PCLK_SAMPLE_RISING|SOCAM_HSYNC_ACTIVE_HIGH| SOCAM_VSYNC_ACTIVE_HIGH|\
											 SOCAM_DATA_ACTIVE_HIGH | SOCAM_DATAWIDTH_8  |SOCAM_MCLK_24MHZ)
#define SENSOR_PREVIEW_W					 640
#define SENSOR_PREVIEW_H					 480
#define SENSOR_PREVIEW_FPS					 15000	   // 15fps 
#define SENSOR_FULLRES_L_FPS				 7500	   // 7.5fps
#define SENSOR_FULLRES_H_FPS				 7500	   // 7.5fps
#define SENSOR_720P_FPS 					 0
#define SENSOR_1080P_FPS					 0

#define SENSOR_REGISTER_LEN 				 1		   // sensor register address bytes
#define SENSOR_VALUE_LEN					 1		   // sensor register value bytes
static unsigned int SensorConfiguration = (CFG_WhiteBalance|CFG_Effect|CFG_Scene);
static unsigned int SensorChipID[] = {SENSOR_ID};
/* Sensor Driver Configuration End */

#define SENSOR_NAME_STRING(a) STR(CONS(SENSOR_NAME, a))
#define SENSOR_NAME_VARFUN(a) CONS(SENSOR_NAME, a)

#define SensorRegVal(a,b) CONS4(SensorReg,SENSOR_REGISTER_LEN,Val,SENSOR_VALUE_LEN)(a,b)
#define sensor_write(client,reg,v) CONS4(sensor_write_reg,SENSOR_REGISTER_LEN,val,SENSOR_VALUE_LEN)(client,(reg),(v))
#define sensor_read(client,reg,v) CONS4(sensor_read_reg,SENSOR_REGISTER_LEN,val,SENSOR_VALUE_LEN)(client,(reg),(v))
#define sensor_write_array generic_sensor_write_array

struct sensor_parameter
{
	unsigned int PreviewDummyPixels;
	unsigned int CaptureDummyPixels;
	unsigned int preview_exposure;
	unsigned short int preview_line_width;
	unsigned short int preview_gain;

	unsigned short int PreviewPclk;
	unsigned short int CapturePclk;
	char awb[6];
};

struct specific_sensor{
	struct generic_sensor common_sensor;
	//define user data below
	struct sensor_parameter parameter;

};

/*
*  The follow setting need been filled.
*  
*  Must Filled:
*  sensor_init_data :				Sensor initial setting;
*  sensor_fullres_lowfps_data : 	Sensor full resolution setting with best auality, recommand for video;
*  sensor_preview_data :			Sensor preview resolution setting, recommand it is vga or svga;
*  sensor_softreset_data :			Sensor software reset register;
*  sensor_check_id_data :			Sensir chip id register;
*
*  Optional filled:
*  sensor_fullres_highfps_data: 	Sensor full resolution setting with high framerate, recommand for video;
*  sensor_720p: 					Sensor 720p setting, it is for video;
*  sensor_1080p:					Sensor 1080p setting, it is for video;
*
*  :::::WARNING:::::
*  The SensorEnd which is the setting end flag must be filled int the last of each setting;
*/

/* Sensor initial setting */
static struct rk_sensor_reg sensor_init_data[] ={
	{0x00, 0x00},
	{0x04, 0x00},
	{0x05, 0x03}, //VGA Output
	{0x07, 0x70},
	{0x10, 0x34},
	{0x11, 0x27},
	{0x12, 0x21},//33}},
	{0x16, 0xc6},
	{0x17, 0xaa},

	// SIV120D 50Hz - 24MHz
	{0x20, 0x00},//10}},
	{0x21, 0xa1},//C3}},
	{0x23, 0x7e},//4D}},
	{0x00, 0x01},
	{0x34, 0x60},

	// SIV120D 50Hz - Still Mode
	{0x00, 0x00},
	{0x24, 0x10},
	{0x25, 0xC3},
	{0x27, 0x4D},

	// Vendor recommended value ##Don't change##
	{0x00, 0x00},
	{0x40, 0x00},
	{0x41, 0x00},
	{0x42, 0x00},
	{0x43, 0x00},

	// AE
	{0x00, 0x01},	
	{0x10, 0x80}, // 6fps at lowlux 		
	{0x11, 0x0a}, // 6fps at lowlux 		
	{0x12, 0x51},//7d}},//80}},//78}},// D65 target 0x74
	{0x13, 0x51},//7d}},//80}},//78}},// CWF target 0x74
	{0x14, 0x51},//7d}},//80}},//78}},// A target ,	0x74
	{0x1E, 0x08},// ini gain, 0x08
	{0x34, 0x7d},//1f}},//0x1A   //STST		   
	{0x40, 0x38},//0x50 //0x38 // Max x6		  

	{0x41, 0x20},	//AG_TOP1	0x28
	{0x42, 0x20},	//AG_TOP0	0x28
	{0x43, 0x00},	//AG_MIN1	0x08
	{0x44, 0x00},	//AG_MIN0	0x08
	{0x45, 0x00},	//G50_dec	0x09
	{0x46, 0x0a},	//G33_dec	0x17
	{0x47, 0x10},	//G25_dec	0x1d
	{0x48, 0x13},	//G20_dec	0x21
	{0x49, 0x15},	//G12_dec	0x23
	{0x4a, 0x18},	//G09_dec	0x24
	{0x4b, 0x1a},	//G06_dec	0x26
	{0x4c, 0x1d},	//G03_dec	0x27
	{0x4d, 0x20},	//G100_inc	0x27
	{0x4e, 0x10},	//G50_inc	0x1a
	{0x4f, 0x0a},	//G33_inc	0x14
	{0x50, 0x08},	//G25_inc	0x11
	{0x51, 0x06},	//G20_inc	0x0f
	{0x52, 0x05},	//G12_inc	0x0d
	{0x53, 0x04},	//G09_inc	0x0c
	{0x54, 0x02},//	//G06_inc	0x0a
	{0x55, 0x01},//	//G03_inc	0x09 

	//AWB					  
	{0x00, 0x02},			   
	{0x10, 0xd0},
	{0x11, 0xc0},
	{0x12, 0x80},
	{0x13, 0x7e},
	{0x14, 0x80},
	{0x15, 0xfe}, // R gain Top
	{0x16, 0x80}, // R gain bottom 
	{0x17, 0xcb}, // B gain Top
	{0x18, 0x80},//70}}, // B gain bottom 0x80
	{0x19, 0x94}, // Cr top value 0x90
	{0x1a, 0x6c}, // Cr bottom value 0x70
	{0x1b, 0x94}, // Cb top value 0x90
	{0x1c, 0x6c}, // Cb bottom value 0x70
	{0x1d, 0x94}, // 0xa0
	{0x1e, 0x6c}, // 0x60
	{0x20, 0xe8}, // AWB luminous top value
	{0x21, 0x30}, // AWB luminous bottom value 0x20
	{0x22, 0xa4},
	{0x23, 0x20},
	{0x25, 0x20},
	{0x26, 0x0f},
	{0x27, 0x60},//10}}, // 60 BRTSRT 
	{0x28, 0xcb},//cb}}, // b4 BRTRGNTOP result 0xb2
	{0x29, 0xc8}, // b0 BRTRGNBOT 
	{0x2a, 0xa0}, // 92 BRTBGNTOP result 0x90
	{0x2b, 0x98},  // 8e BRTBGNBOT
	{0x2c, 0x88}, // RGAINCONT
	{0x2d, 0x88}, // BGAINCONT

	{0x30, 0x00},
	{0x31, 0x10},
	{0x32, 0x00},
	{0x33, 0x10},
	{0x34, 0x02},
	{0x35, 0x76},
	{0x36, 0x01},
	{0x37, 0xd6},
	{0x40, 0x01},
	{0x41, 0x04},
	{0x42, 0x08},
	{0x43, 0x10},
	{0x44, 0x12},
	{0x45, 0x35},
	{0x46, 0x64},
	{0x50, 0x33},
	{0x51, 0x20},
	{0x52, 0xe5},
	{0x53, 0xfb},
	{0x54, 0x13},
	{0x55, 0x26},
	{0x56, 0x07},
	{0x57, 0xf5},
	{0x58, 0xea},
	{0x59, 0x21},

	{0x62, 0x88}, // G gain

	{0x63, 0xb3}, // R D30 to D20
	{0x64, 0xc3},//b8}}, // c3 B D30 to D20
	{0x65, 0xb3}, // R D20 to D30
	{0x66, 0xc3},//b8}}, // c3 B D20 to D30

	{0x67, 0xdd}, // R D65 to D30
	{0x68, 0xa0}, // B D65 to D30
	{0x69, 0xdd}, // R D30 to D65
	{0x6a, 0xa0}, // B D30 to D65

	//IDP
	{0x00, 0x03},
	{0x10, 0xFF}, // IDP function enable
	{0x11, 0x19},//1D}}, // PCLK polarity
	{0x12, 0x7d}, // Y,Cb,Cr order sequence
	{0x14, 0x04}, // don't change
	{0x8c, 0x10},	//CMA select


	// DPCNR				 
	{0x17, 0x28},	// DPCNRCTRL
	{0x18, 0x04},	// DPTHR
	{0x19, 0x50},	// C DP Number {{ Normal [7:6] Dark [5:4] }} | [3:0] DPTHRMIN
	{0x1A, 0x50},	// G DP Number {{ Normal [7:6] Dark [5:4] }} | [3:0] DPTHRMAX
	{0x1B, 0x12},	// DPTHRSLP{{ [7:4] @ Normal | [3:0] @ Dark }}
	{0x1C, 0x00},	// NRTHR
	{0x1D, 0x0f},//08}},	// [5:0] NRTHRMIN 0x48
	{0x1E, 0x0f},//08}},	// [5:0] NRTHRMAX 0x48
	{0x1F, 0x3f},//28}},	// NRTHRSLP{{ [7:4] @ Normal | [3:0] @ Dark }}, 0x2f
	{0x20, 0x04},	// IllumiInfo STRTNOR
	{0x21, 0x0f},	// IllumiInfo STRTDRK

	// Gamma	
#if 0
	{0x30, 0x00},	//0x0 
	{0x31, 0x04},	//0x3 
	{0x32, 0x0b},	//0xb 
	{0x33, 0x24},	//0x1f
	{0x34, 0x49},	//0x43
	{0x35, 0x66},	//0x5f
	{0x36, 0x7C},	//0x74
	{0x37, 0x8D},	//0x85
	{0x38, 0x9B},	//0x94
	{0x39, 0xAA},	//0xA2
	{0x3a, 0xb6},	//0xAF
	{0x3b, 0xca},	//0xC6
	{0x3c, 0xdc},	//0xDB
	{0x3d, 0xef},	//0xEF
	{0x3e, 0xf8},	//0xF8
	{0x3f, 0xFF},	//0xFF
#else
	{0x30, 0x00},	//0x0 
	{0x31, 0x03},//0x080x31, 0x04 },//0x08   0x03
	{0x32, 0x0b},//0x100x32, 0x0b },//0x10   0x08
	{0x33, 0x1e},//0x1B0x33, 0x24 },//0x1B   0x16
	{0x34, 0x46},//0x370x34, 0x49 },//0x37   0x36
	{0x35, 0x62},//0x4D0x35, 0x66 },//0x4D   0x56
	{0x36, 0x78},//0x600x36, 0x7C },//0x60   0x77
	{0x37, 0x8b},//0x720x37, 0x8D },//0x72   0x88
	{0x38, 0x9b},//0x820x38, 0x9B },//0x82   0x9a
	{0x39, 0xa8},//0x910x39, 0xAA },//0x91   0xA9
	{0x3a, 0xb6},//0xA00x3a, 0xb6 },//0xA0   0xb5
	{0x3b, 0xcc},//0xBA0x3b, 0xca },//0xBA   0xca
	{0x3c, 0xdf},//0xD30x3c, 0xdc },//0xD3   0xdd
	{0x3d, 0xf0},//0xEA0x3d, 0xef },//0xEA   0xee
#endif
	// Shading Register Setting }}, 								 
	{0x40, 0x0a},													  
	{0x41, 0xdc},													  
	{0x42, 0x77},													  
	{0x43, 0x66},													  
	{0x44, 0x55},													  
	{0x45, 0x44},													  
	{0x46, 0x11},	// left R gain[7:4], right R gain[3:0]			   
	{0x47, 0x11},	// top R gain[7:4], bottom R gain[3:0]			   
	{0x48, 0x10},	// left Gr gain[7:4], right Gr gain[3:0] 0x21			
	{0x49, 0x00},	// top Gr gain[7:4], bottom Gr gain[3:0]		   
	{0x4a, 0x00},	// left Gb gain[7:4], right Gb gain[3:0] 0x02		   
	{0x4b, 0x00},	// top Gb gain[7:4], bottom Gb gain[3:0]		   
	{0x4c, 0x00},	// left B gain[7:4], right B gain[3:0]			   
	{0x4d, 0x11},	// top B gain[7:4], bottom B gain[3:0]			   
	{0x4e, 0x04},	// X-axis center high[3:2], Y-axis center high[1:0]
	{0x4f, 0x98},	// X-axis center low[7:0] 0x50					   
	{0x50, 0xd8},	// Y-axis center low[7:0] 0xf6					   
	{0x51, 0x80},	// Shading Center Gain							   
	{0x52, 0x00},	// Shading R Offset 							   
	{0x53, 0x00},	// Shading Gr Offset							   
	{0x54, 0x00},	// Shading Gb Offset							   
	{0x55, 0x00},	// Shading B Offset   

	// Interpolation
	{0x60, 0x57},  //INT outdoor condition
	{0x61, 0x57},  //INT normal condition

	{0x62, 0x77},  //ASLPCTRL 7:4 GE, 3:0 YE
	{0x63, 0x38},  //YDTECTRL {{YE}} [7] fixed,
	{0x64, 0x38},  //GPEVCTRL {{GE}} [7] fixed,

	{0x66, 0x0c},  //SATHRMIN
	{0x67, 0xff},
	{0x68, 0x04},  //SATHRSRT
	{0x69, 0x08},  //SATHRSLP

	{0x6a, 0xaf},  //PTDFATHR [7] fixed, [5:0] value
	{0x6b, 0x58},  //PTDLOTHR [6] fixed, [5:0] value
	{0x6d, 0x88},  //PTDLOTHR [6] fixed, [5:0] value

	{0x8f, 0x00},  //Cb/Cr coring

	// Color matrix {{D65}} - Daylight	
#if 0		   
	{0x71, 0x3d},	//0x40		   
	{0x72, 0xcd},	//0xb9		   
	{0x73, 0xf6},	//0x07		   
	{0x74, 0x14},	//0x15		   
	{0x75, 0x2c},	//0x21		   
	{0x76, 0x00},	//0x0a		   
	{0x77, 0xf6},	//0xf8		 
	{0x78, 0xc7},	//0xc5		   
	{0x79, 0x43},	//0x46		   
								   
	// Color matrix {{D30}} - CWF				   
	{0x7a, 0x3d},	//0x3a		 
	{0x7b, 0xcd},	//0xcd		 
	{0x7c, 0xf6},	//0xfa		 
	{0x7d, 0x14},	//0x12		 
	{0x7e, 0x2c},	//0x2c		 
	{0x7f, 0x00},	//0x02		 
	{0x80, 0xf6},	//0xf7		 
	{0x81, 0xc7},	//0xc7		 
	{0x82, 0x43},	//0x42		 
								   
	// Color matrix {{D20}} - A 				   
	{0x83, 0x4d},	//0x38		   
	{0x84, 0xc0},	//0xc4		   
	{0x85, 0xf4},	//0x04		 
	{0x86, 0x0a},	//0x07		   
	{0x87, 0x31},	//0x25		   
	{0x88, 0x03},	//0x14		   
	{0x89, 0xf1},	//0xf0		 
	{0x8a, 0xbd},	//0xc2		   
	{0x8b, 0x53},	//0x4f		 
#else
	{0x71,	0x42},	//0x40       
	{0x72,	0xbf},	//0xb9      
	{0x73,	0x00},	//0x07      
	{0x74,	0x0f},	//0x15       
	{0x75,	0x31},	//0x21       
	{0x76,	0x00},	//0x0a       
	{0x77,	0x00},	//0xf8     
	{0x78,	0xbc},	//0xc5       
	{0x79,	0x44},	//0x46       

	// Color matrix (D30) - CWF
	{0x7a,	0x3a},	//0x38	
	{0x7b,	0xcd},  //0xbc 
	{0x7c,	0xfa},  //0x0c 
	{0x7d,	0x12},  //0x14 
	{0x7e,	0x2c},  //0x1e 
	{0x7f,	0x02},  //0x0e 
	{0x80,	0xf7},  //0xf6 
	{0x81,	0xc7},  //0xcd 
	{0x82,	0x42},  //0x3d 

	// Color matrix (D20) - A
	{0x83,	0x3a},	//0x38         
	{0x84,	0xcd},  //0xc4       
	{0x85,	0xfa},  //0x04    
	{0x86,	0x12},  //0x07      
	{0x87,	0x2c},  //0x25       
	{0x88,	0x02},  //0x14       
	{0x89,	0xf7},  //0xf0   
	{0x8a,	0xc7},  //0xc2       
	{0x8b,	0x42},  //0x4f     
#endif
	{0x8c, 0x10},
	{0x8d, 0x04},	//programmable edge
	{0x8e, 0x02},	//PROGEVAL
	{0x8f, 0x00},	//Cb/Cr coring

	{0x90, 0x10},//20}},	//GEUGAIN
	{0x91, 0x10},//20}},	//GEDGAIN
	{0x92, 0x02},//F0}},	//Ucoring [7:4] max, [3:0] min
	//{0x94, 0x02},//00}},	//Uslope {{1/128}}
	{0x96, 0x02},//f0}},	//Dcoring [7:4] max, [3:0] min
	//{0x98, 0x00},	//Dslope {{1/128}}

	//{0x9a, 0x08},
	//{0x9b, 0x18},

	{0x9f, 0x0c}, //YEUGAIN
	{0xa0, 0x0c}, //YEDGAIN
	{0xa1, 0x22}, //Yecore [7:4]upper [3:0]down

	{0xa9, 0x12}, // Cr saturation 0x12
	{0xaa, 0x12}, // Cb saturation 0x12
	{0xab, 0x82},//00}},//82}}, // Brightness
	{0xae, 0x40}, // Hue
	{0xaf, 0x86}, // Hue
	{0xb9, 0x10}, // 0x20 lowlux color
	{0xba, 0x20}, // 0x10 lowlux color	
	// {0xbc, 0x0b},
	// {0xbd, 0x14},
	// {0xbe, 0x10},
	// {0xbf, 0x20},

	// inverse color space conversion
	{0xcc, 0x40},
	{0xcd, 0x00},
	{0xce, 0x58},
	{0xcf, 0x40},
	{0xd0, 0xea},
	{0xd1, 0xd3},
	{0xd2, 0x40},
	{0xd3, 0x6f},
	{0xd4, 0x00},

	// ee/nr				   
	{0xd9, 0x0b},
	{0xda, 0x1f},
	{0xdb, 0x05},
	{0xdc, 0x08},
	{0xdd, 0x3C},		   
	{0xde, 0xF9},	//NOIZCTRL	

	//window
	{0xc0, 0x24},
	{0xC1, 0x00},
	{0xC2, 0x80},
	{0xC3, 0x00},
	{0xC4, 0xE0},
	// memory speed 		   
	{0xe5, 0x15},
	{0xe6, 0x20},
	{0xe7, 0x04},

	{0x00, 0x02},
	{0x10, 0xd0},

	//Sensor On 			  
	{0x00, 0x00},
	{0x03, 0x05},
	SensorEnd
};

/* Senor full resolution setting: recommand for capture */
static struct rk_sensor_reg sensor_fullres_lowfps_data[] ={
	SensorEnd

};

/* Senor full resolution setting: recommand for video */
static struct rk_sensor_reg sensor_fullres_highfps_data[] ={
	SensorEnd
};
/* Preview resolution setting*/
static struct rk_sensor_reg sensor_preview_data[] =
{

	SensorEnd
};
/* 1280x720 */
static struct rk_sensor_reg sensor_720p[]={
	SensorEnd
};

/* 1920x1080 */
static struct rk_sensor_reg sensor_1080p[]={
	SensorEnd
};


static struct rk_sensor_reg sensor_softreset_data[]={
	SensorEnd
};

static struct rk_sensor_reg sensor_check_id_data[]={
	SensorEnd
};
/*
*  The following setting must been filled, if the function is turn on by CONFIG_SENSOR_xxxx
*/
static struct rk_sensor_reg sensor_WhiteB_Auto[]=
{
    {0x00,  0x02}, 
    {0x10,  0xd0}, 
    {0x60,  0xa0}, 
    {0x61,  0xa0},
    	SensorEnd
};
/* Cloudy Colour Temperature : 6500K - 8000K  */
static	struct rk_sensor_reg sensor_WhiteB_Cloudy[]=
{
    {0x00,  0x02}, 
    {0x10, 0x00},  // disable AWB
    {0x60, 0xb4}, 
    {0x61, 0x74}, 
    	SensorEnd

};
/* ClearDay Colour Temperature : 5000K - 6500K	*/
static	struct rk_sensor_reg sensor_WhiteB_ClearDay[]=
{
    {0x00,  0x02}, 
    {0x10, 0x00},   // disable AWB
    {0x60, 0xd8}, 
    {0x61, 0x90},
   	SensorEnd
    
};
/* Office Colour Temperature : 3500K - 5000K  */
static	struct rk_sensor_reg sensor_WhiteB_TungstenLamp1[]=
{
    {0x00,  0x02}, 
    {0x10, 0x00},  // disable AWB
    {0x60, 0x80},
    {0x61, 0xe0},
    SensorEnd
};
/* Home Colour Temperature : 2500K - 3500K	*/
static	struct rk_sensor_reg sensor_WhiteB_TungstenLamp2[]=
{
    {0x00,  0x02}, 
    {0x10, 0x00},  // disable AWB
    {0x60, 0xb8},
    {0x61, 0xcc},
    	SensorEnd
};
static struct rk_sensor_reg *sensor_WhiteBalanceSeqe[] = {sensor_WhiteB_Auto, sensor_WhiteB_TungstenLamp1,sensor_WhiteB_TungstenLamp2,
	sensor_WhiteB_ClearDay, sensor_WhiteB_Cloudy,NULL,
};

static	struct rk_sensor_reg sensor_Brightness0[]=
{
    // Brightness -2
    {0x00,  0x03}, 
    {0xab,  0xb0}, 
    	SensorEnd 
};

static struct rk_sensor_reg sensor_Brightness1[]=
{
    // Brightness -1
    {0x00,  0x03}, 
    {0xab,  0xa0}, 
    	SensorEnd
};

static struct rk_sensor_reg sensor_Brightness2[]=
{
    //  Brightness 0
    {0x00,  0x03}, 
    {0xab,  0xb0},
   	SensorEnd
};

static  struct rk_sensor_reg sensor_Brightness3[]=
{
    // Brightness +1
    {0x00,  0x03}, 
    {0xab,  0x00}, 
   	SensorEnd
};

static  struct rk_sensor_reg sensor_Brightness4[]=
{
    //  Brightness +2
    {0x00,  0x03}, 
    {0xab,  0x10}, 
   	SensorEnd
};

static  struct rk_sensor_reg sensor_Brightness5[]=
{
    //  Brightness +3
    {0x00,  0x03}, 
    {0xab,  0x20}, 
    	SensorEnd
};
static struct rk_sensor_reg *sensor_BrightnessSeqe[] = {sensor_Brightness0, sensor_Brightness1, sensor_Brightness2, sensor_Brightness3,
	sensor_Brightness4, sensor_Brightness5,NULL,
};

static	struct rk_sensor_reg sensor_Effect_Normal[] =
{
   {0x00,0x03}, 
   // {0x90,0x30},
    //{0x91,0x48},
    {0xb6,0x00},
  	SensorEnd
};

static	struct rk_sensor_reg sensor_Effect_WandB[] =
{
   //mono 
    {0x00,0x03}, 
    {0x90,0x14},
    {0x91,0x18},
    {0xb6,0x40},
  	SensorEnd
};

static	struct rk_sensor_reg sensor_Effect_Sepia[] =
{
    {0x00,0x03}, 
    {0x90, 0x14},
    {0x91, 0x18},
    {0xB6, 0x80},
    {0xB7, 0x58},
    {0xB8, 0x98},
 	SensorEnd 
};

static	struct rk_sensor_reg sensor_Effect_Negative[] =
{
   //Negative
    {0x00,0x03}, 
    {0x90, 0x18},
    {0x91, 0x18},
    {0xB6, 0x20},
   	SensorEnd
};
static	struct rk_sensor_reg sensor_Effect_Bluish[] =
{
    {0x00,0x03}, 
    {0x90, 0x14},
    {0x91, 0x18},
    {0xB6, 0x80},
    {0xB7, 0xb8},
    {0xB8, 0x50},
  	SensorEnd 
};

static	struct rk_sensor_reg sensor_Effect_Green[] =
{
   //  Greenish
    {0x00,0x03}, 
    {0x90, 0x14},
    {0x91, 0x18},
    {0xB6, 0x80},
    {0xB7, 0x68},
    {0xB8, 0x68},
    	SensorEnd
};
static struct rk_sensor_reg *sensor_EffectSeqe[] = {sensor_Effect_Normal, sensor_Effect_WandB, sensor_Effect_Negative,sensor_Effect_Sepia,
	sensor_Effect_Bluish, sensor_Effect_Green,NULL,
};

//#if CONFIG_SENSOR_Exposure
static	struct rk_sensor_reg sensor_Exposure0[]=
{  /* 
    {0x00,0x02},
    {0x12,0x34},////44},
    {0x14,0x30},//44},
    {0xff,0xff}
    */
   

	SensorEnd

};

static	struct rk_sensor_reg sensor_Exposure1[]=
{
	//-2
    	SensorEnd



	/*
    {0x00,0x02},
    {0x12,0x44},//54},
    {0x14,0x40},//54},
    {0xff,0xff}
    */
};

static	struct rk_sensor_reg sensor_Exposure2[]=
{
   
    	SensorEnd


	/*
    {0x00,0x02},
    {0x12,0x54},//54},
    {0x14,0x50},//54},
    {0xff,0xff}
    */
    
};

static	struct rk_sensor_reg sensor_Exposure3[]=
{
	//default
    
   	SensorEnd


	/*
    {0x00,0x02},
    {0x12,0x64},//78},
    {0x14,0x60},//78},
    {0xff,0xff}
    */
};

static	struct rk_sensor_reg sensor_Exposure4[]=
{
   
   	SensorEnd 



	/*
    {0x00,0x02},
    {0x12,0x74},//84},
    {0x14,0x70},//84},
    {0xff,0xff}
    */
};

static	struct rk_sensor_reg sensor_Exposure5[]=
{
	// 2

   	SensorEnd


	/*
    {0x00,0x02},
    {0x12,0x84},//94},
    {0x14,0x80},//94},
    {0xff,0xff}
    */
};

static	struct rk_sensor_reg sensor_Exposure6[]=
{
	
       	SensorEnd 


	/*
    {0x00,0x02},
    {0x12,0x94},//a4},
    {0x14,0x90},//a4},
    {0xff,0xff}
    */
};

static struct rk_sensor_reg *sensor_ExposureSeqe[] = {sensor_Exposure0, sensor_Exposure1, sensor_Exposure2, sensor_Exposure3,
    sensor_Exposure4, sensor_Exposure5,sensor_Exposure6,NULL,
};

static	struct rk_sensor_reg sensor_Saturation0[]=
{
	SensorEnd
};

static	struct rk_sensor_reg sensor_Saturation1[]=
{
	SensorEnd
};

static	struct rk_sensor_reg sensor_Saturation2[]=
{
	SensorEnd
};
static struct rk_sensor_reg *sensor_SaturationSeqe[] = {sensor_Saturation0, sensor_Saturation1, sensor_Saturation2, NULL,};

static	struct rk_sensor_reg sensor_Contrast0[]=
{
    //  Brightness +3
   	SensorEnd
};

static	struct rk_sensor_reg sensor_Contrast1[]=
{
    //  Brightness +3
    	SensorEnd
};

static	struct rk_sensor_reg sensor_Contrast2[]=
{
    //  Brightness +3
    	SensorEnd
};

static	struct rk_sensor_reg sensor_Contrast3[]=
{
    //  Brightness +3
    	SensorEnd
};

static	struct rk_sensor_reg sensor_Contrast4[]=
{
    //  Brightness +3
   	SensorEnd
};


static	struct rk_sensor_reg sensor_Contrast5[]=
{
    //  Brightness +3
    	SensorEnd
};

static	struct rk_sensor_reg sensor_Contrast6[]=
{
    //  Brightness +3
   	SensorEnd
};
static struct rk_sensor_reg *sensor_ContrastSeqe[] = {sensor_Contrast0, sensor_Contrast1, sensor_Contrast2, sensor_Contrast3,
    sensor_Contrast4, sensor_Contrast5, sensor_Contrast6, NULL,
};
static	struct rk_sensor_reg sensor_SceneAuto[] =
{
    {0x00, 0x01},
	{0x40, 0x40},	
	{0x00, 0x03},
	{0xab, 0x00},
	
	SensorEnd
};

static	struct rk_sensor_reg sensor_SceneNight[] =
{
    {0x00, 0x01},
	{0x40, 0x50},	
	{0x00, 0x03},
	{0xab, 0x30},
		SensorEnd
};
static struct rk_sensor_reg *sensor_SceneSeqe[] = {sensor_SceneAuto, sensor_SceneNight,NULL,};

static struct rk_sensor_reg sensor_Zoom0[] =
{
	SensorEnd
		
};

static struct rk_sensor_reg sensor_Zoom1[] =
{
	SensorEnd
};

static struct rk_sensor_reg sensor_Zoom2[] =
{
	SensorEnd
};


static struct rk_sensor_reg sensor_Zoom3[] =
{
	SensorEnd
};
static struct rk_sensor_reg *sensor_ZoomSeqe[] = {sensor_Zoom0, sensor_Zoom1, sensor_Zoom2, sensor_Zoom3, NULL,};

/*
* User could be add v4l2_querymenu in sensor_controls by new_usr_v4l2menu
*/
static struct v4l2_querymenu sensor_menus[] =
{
};
/*
* User could be add v4l2_queryctrl in sensor_controls by new_user_v4l2ctrl
*/
static struct sensor_v4l2ctrl_usr_s sensor_controls[] =
{
};

//MUST define the current used format as the first item   
static struct rk_sensor_datafmt sensor_colour_fmts[] = {
	{V4L2_MBUS_FMT_YUYV8_2X8, V4L2_COLORSPACE_JPEG} 
};
static struct soc_camera_ops sensor_ops;


/*
**********************************************************
* Following is local code:
* 
* Please codeing your program here 
**********************************************************
*/
/*
**********************************************************
* Following is callback
* If necessary, you could coding these callback
**********************************************************
*/
/*
* the function is called in open sensor  
*/
static int sensor_activate_cb(struct i2c_client *client)
{
	
	return 0;
}
/*
* the function is called in close sensor
*/
static int sensor_deactivate_cb(struct i2c_client *client)
{
	
	return 0;
}
/*
* the function is called before sensor register setting in VIDIOC_S_FMT  
*/
static int sensor_s_fmt_cb_th(struct i2c_client *client,struct v4l2_mbus_framefmt *mf, bool capture)
{

	return 0;
}
/*
* the function is called after sensor register setting finished in VIDIOC_S_FMT  
*/
static int sensor_s_fmt_cb_bh (struct i2c_client *client,struct v4l2_mbus_framefmt *mf, bool capture)
{
	return 0;
}
static int sensor_try_fmt_cb_th(struct i2c_client *client,struct v4l2_mbus_framefmt *mf)
{
	return 0;
}

static int sensor_softrest_usr_cb(struct i2c_client *client,struct rk_sensor_reg *series)
{
	
    return 0;
}
static int sensor_check_id_usr_cb(struct i2c_client *client,struct rk_sensor_reg *series)
{
	char pid = 0;
	int ret = 0;
	struct generic_sensor *sensor = to_generic_sensor(client);
  	
	ret = sensor_write(client, 0x00, 0x00);  //before read id should write 0xfc
	msleep(20);
	ret = sensor_read(client, 0x01, &pid);
	if (ret != 0) {
		SENSOR_TR("%s read chip id high byte failed\n",SENSOR_NAME_STRING());
		ret = -ENODEV;
	}
	SENSOR_DG("\n %s  pid = 0x%x\n", SENSOR_NAME_STRING(), pid);
	//while(pid != SENSOR_ID) {
    //	ret = sensor_read(client, 0x01, &pid);
  	//	printk("\n read again***hkw***: %s  pid = 0x%x;SENSOR_ID = 0x%x;reg:0x01\n", SENSOR_NAME_STRING(), pid,SENSOR_ID);
    //}
	if (pid == SENSOR_ID) {
		sensor->model = SENSOR_V4L2_IDENT;
	} else {
		SENSOR_TR("error: %s mismatched   pid = 0x%x\n", SENSOR_NAME_STRING(), pid);
		ret = -ENODEV;
	}
	return pid;
}
static int sensor_suspend(struct soc_camera_device *icd, pm_message_t pm_msg)
{
	//struct i2c_client *client = to_i2c_client(to_soc_camera_control(icd));
		
	if (pm_msg.event == PM_EVENT_SUSPEND) {
		SENSOR_DG("Suspend");
		
	} else {
		SENSOR_TR("pm_msg.event(0x%x) != PM_EVENT_SUSPEND\n",pm_msg.event);
		return -EINVAL;
	}
	return 0;
}

static int sensor_resume(struct soc_camera_device *icd)
{

	SENSOR_DG("Resume");

	return 0;

}
static int sensor_mirror_cb (struct i2c_client *client, int mirror)
{
	char val;
	int err = 0;
	
	SENSOR_DG("mirror: %d",mirror);
	if (mirror) {
		
		//{0xfe , 0x00}, // set page0
		
		//{0x14 , 0x13},	//0x10
		//-------------H_V_Switch(4)---------------//
		/*	1:	// normal
					{0x14 , 0x10},			
			2:	// IMAGE_H_MIRROR
					{0x14 , 0x11},
					
			3:	// IMAGE_V_MIRROR
					{0x14 , 0x12},
					
			4:	// IMAGE_HV_MIRROR
					{0x14 , 0x13},*/										
		sensor_write(client, 0x00, 0);
		err = sensor_read(client, 0x04, &val);
		if (err == 0) {
			if((val & 0x1) == 0)
				err = sensor_write(client, 0x04, (val |0x00));
			else 
				err = sensor_write(client, 0x04, (val & 0x01));
    }
	} else {
		//do nothing
	}

	return err;    
}
/*
* the function is v4l2 control V4L2_CID_HFLIP callback	
*/
static int sensor_v4l2ctrl_mirror_cb(struct soc_camera_device *icd, struct sensor_v4l2ctrl_info_s *ctrl_info, 
													 struct v4l2_ext_control *ext_ctrl)
{
	struct i2c_client *client = to_i2c_client(to_soc_camera_control(icd));

	if (sensor_mirror_cb(client,ext_ctrl->value) != 0)
		SENSOR_TR("sensor_mirror failed, value:0x%x",ext_ctrl->value);
	
	SENSOR_DG("sensor_mirror success, value:0x%x",ext_ctrl->value);
	return 0;
}

static int sensor_flip_cb(struct i2c_client *client, int flip)
{
	char val;
	int err = 0;	

	SENSOR_DG("flip: %d",flip);
	if (flip) {
		
		//{0xfe , 0x00}, // set page0
		
		//{0x14 , 0x13},	//0x10
		//-------------H_V_Switch(4)---------------//
		/*	1:	// normal
					{0x14 , 0x10},			
			2:	// IMAGE_H_MIRROR
					{0x14 , 0x11},
					
			3:	// IMAGE_V_MIRROR
					{0x14 , 0x12},
					
			4:	// IMAGE_HV_MIRROR
					{0x14 , 0x13},*/	
		sensor_write(client, 0x00, 0);
		err = sensor_read(client, 0x04, &val);
		if (err == 0) {
			if((val & 0x2) == 0)
				err = sensor_write(client, 0x04, (val |0x00));
			else 
				err = sensor_write(client, 0x04, (val & 0x02));
		}
	} else {
		//do nothing
	}
	return err;    
}

static int sensor_v4l2ctrl_flip_cb(struct soc_camera_device *icd, struct sensor_v4l2ctrl_info_s *ctrl_info, 
													 struct v4l2_ext_control *ext_ctrl)
{
	struct i2c_client *client = to_i2c_client(to_soc_camera_control(icd));

	if (sensor_flip_cb(client,ext_ctrl->value) != 0)
		SENSOR_TR("sensor_flip failed, value:0x%x",ext_ctrl->value);
	
	SENSOR_DG("sensor_flip success, value:0x%x",ext_ctrl->value);
	return 0;
}
/*
* the functions are focus callbacks
*/
static int sensor_focus_init_usr_cb(struct i2c_client *client){
	return 0;
}

static int sensor_focus_af_single_usr_cb(struct i2c_client *client){
	return 0;
}

static int sensor_focus_af_near_usr_cb(struct i2c_client *client){
	return 0;
}

static int sensor_focus_af_far_usr_cb(struct i2c_client *client){
	return 0;
}
static int sensor_focus_af_specialpos_usr_cb(struct i2c_client *client,int pos){
	return 0;
	}

static int sensor_focus_af_const_usr_cb(struct i2c_client *client){
	return 0;
}
static int sensor_focus_af_const_pause_usr_cb(struct i2c_client *client)
{
    return 0;
}
static int sensor_focus_af_close_usr_cb(struct i2c_client *client){
	return 0;
}

static int sensor_focus_af_zoneupdate_usr_cb(struct i2c_client *client, int *zone_tm_pos)
{
    return 0;
}

/*
face defect call back
*/
static int	sensor_face_detect_usr_cb(struct i2c_client *client,int on){
	return 0;
}

/*
*	The function can been run in sensor_init_parametres which run in sensor_probe, so user can do some
* initialization in the function. 
*/
static void sensor_init_parameters_user(struct specific_sensor* spsensor,struct soc_camera_device *icd)
{
	return;
}

/*
* :::::WARNING:::::
* It is not allowed to modify the following code
*/

sensor_init_parameters_default_code();

sensor_v4l2_struct_initialization();

sensor_probe_default_code();

sensor_remove_default_code();

sensor_driver_default_module_code();



