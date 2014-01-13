#ifdef CONFIG_VIDEO_RK29
#include <plat/rk_camera.h>
/* Notes:

Simple camera device registration:

       new_camera_device(sensor_name,\       // sensor name, it is equal to CONFIG_SENSOR_X
                          face,\              // sensor face information, it can be back or front
                          pwdn_io,\           // power down gpio configuration, it is equal to CONFIG_SENSOR_POWERDN_PIN_XX
                          flash_attach,\      // sensor is attach flash or not
                          mir,\               // sensor image mirror and flip control information
                          i2c_chl,\           // i2c channel which the sensor attached in hardware, it is equal to CONFIG_SENSOR_IIC_ADAPTER_ID_X
                          cif_chl)  \         // cif channel which the sensor attached in hardware, it is equal to CONFIG_SENSOR_CIF_INDEX_X

Comprehensive camera device registration:

      new_camera_device_ex(sensor_name,\
                             face,\
                             ori,\            // sensor orientation, it is equal to CONFIG_SENSOR_ORIENTATION_X
                             pwr_io,\         // sensor power gpio configuration, it is equal to CONFIG_SENSOR_POWER_PIN_XX
                             pwr_active,\     // sensor power active level, is equal to CONFIG_SENSOR_RESETACTIVE_LEVEL_X
                             rst_io,\         // sensor reset gpio configuration, it is equal to CONFIG_SENSOR_RESET_PIN_XX
                             rst_active,\     // sensor reset active level, is equal to CONFIG_SENSOR_RESETACTIVE_LEVEL_X
                             pwdn_io,\
                             pwdn_active,\    // sensor power down active level, is equal to CONFIG_SENSOR_POWERDNACTIVE_LEVEL_X
                             flash_attach,\
                             res,\            // sensor resolution, this is real resolution or resoltuion after interpolate
                             mir,\
                             i2c_chl,\
                             i2c_spd,\        // i2c speed , 100000 = 100KHz
                             i2c_addr,\       // the i2c slave device address for sensor
                             cif_chl,\
                             mclk)\           // sensor input clock rate, 24 or 48
                          
*/
static struct rkcamera_platform_data new_camera[] = { 
#if defined(CONFIG_MACH_RK3026_PHONEPAD_780)
	new_camera_device(RK29_CAM_SENSOR_GC2035,
		                        back,
		                        0,
		                        0,
		                        0,
		                        1,
		                        0),
	new_camera_device(RK29_CAM_SENSOR_GC0308,
		                        front,
		                        RK30_PIN3_PB3,
		                        0,
		                        0,
		                        1,
		                        0), 
	new_camera_device_end
#else
	/*		
	new_camera_device(RK29_CAM_SENSOR_GC0308,
			 back,
			 INVALID_GPIO,
			 0,
			 1,
			 1,
			 0),
	*/	
#ifdef CONFIG_SOC_CAMERA_SP0838		
	new_camera_device(RK29_CAM_SENSOR_SP0838,
			back,
			INVALID_GPIO,
			0,
			0,
			1,
			0),
#elif defined( CONFIG_SOC_CAMERA_GC2035 )
	new_camera_device(RK29_CAM_SENSOR_GC2035,
			back,
			INVALID_GPIO,
			0,
			0,
			1,
			0),
#elif defined( CONFIG_SOC_CAMERA_SP0A19 )
	new_camera_device(RK29_CAM_SENSOR_SP0A19,
			back,
			INVALID_GPIO,
			0,
			0,
			1,
			0),
#elif defined( CONFIG_SOC_CAMERA_GC0329 )
	new_camera_device(RK29_CAM_SENSOR_GC0329,
			back,
			INVALID_GPIO,
			0,
			0,
			1,
			0),
#elif defined( CONFIG_SOC_CAMERA_GC0309 )
	new_camera_device(RK29_CAM_SENSOR_GC0309,
			back,
			INVALID_GPIO,
			0,
			0,
			1,
			0),
#else
	new_camera_device(RK29_CAM_SENSOR_GC2035,
			back,
			INVALID_GPIO,
			0,
			0,
			1,
			0),
#endif
#if defined( CONFIG_SOC_CAMERA_SP0838 )		
	new_camera_device(RK29_CAM_SENSOR_SP0838,
			 front,
			 RK30_PIN3_PB3,
			 0,
			 0,
			 1,
			 0),
#elif defined( CONFIG_SOC_CAMERA_GC0308 )
	new_camera_device(RK29_CAM_SENSOR_GC0308,
	                front,
	                RK30_PIN3_PB3,
	                0,
	                0,
	                1,
	                0),
#elif defined( CONFIG_SOC_CAMERA_GC0329 )
	new_camera_device(RK29_CAM_SENSOR_GC0329,
			 front,
			 RK30_PIN3_PB3,
			 0,
			 0,
			 1,
			 0),
#elif defined( CONFIG_SOC_CAMERA_SP0A19 )
	new_camera_device(RK29_CAM_SENSOR_SP0A19,
			 front,
			 RK30_PIN3_PB3,
			 0,
			 0,
			 1,
			 0),
#elif defined( CONFIG_SOC_CAMERA_GC0311 )
	new_camera_device(RK29_CAM_SENSOR_GC0311,
			 front,
			 RK30_PIN3_PB3,
			 0,
			 0,
			 1,
			 0),
#else
	new_camera_device(RK29_CAM_SENSOR_GC0308,
	                front,
	                RK30_PIN3_PB3,
	                0,
	                0,
	                1,
	                0),
#endif
	new_camera_device_end
#endif
};
/*---------------- Camera Sensor Macro Define Begin  ------------------------*/
/*---------------- Camera Sensor Configuration Macro Begin ------------------------*/
#define CONFIG_SENSOR_0 RK29_CAM_SENSOR_GC2035						/* back camera sensor */
#define CONFIG_SENSOR_IIC_ADDR_0		0x00
#define CONFIG_SENSOR_IIC_ADAPTER_ID_0	  1
#define CONFIG_SENSOR_CIF_INDEX_0                    0
#define CONFIG_SENSOR_ORIENTATION_0 	  90
#define CONFIG_SENSOR_POWER_PIN_0		  INVALID_GPIO
#define CONFIG_SENSOR_RESET_PIN_0		  INVALID_GPIO
#define CONFIG_SENSOR_POWERDN_PIN_0 	  RK30_PIN3_PB3
#define CONFIG_SENSOR_FALSH_PIN_0		  INVALID_GPIO
#define CONFIG_SENSOR_POWERACTIVE_LEVEL_0 RK29_CAM_POWERACTIVE_L
#define CONFIG_SENSOR_RESETACTIVE_LEVEL_0 RK29_CAM_RESETACTIVE_L
#define CONFIG_SENSOR_POWERDNACTIVE_LEVEL_0 RK29_CAM_POWERDNACTIVE_H
#define CONFIG_SENSOR_FLASHACTIVE_LEVEL_0 RK29_CAM_FLASHACTIVE_H

#define CONFIG_SENSOR_QCIF_FPS_FIXED_0		15000
#define CONFIG_SENSOR_240X160_FPS_FIXED_0   15000
#define CONFIG_SENSOR_QVGA_FPS_FIXED_0		15000
#define CONFIG_SENSOR_CIF_FPS_FIXED_0		15000
#define CONFIG_SENSOR_VGA_FPS_FIXED_0		15000
#define CONFIG_SENSOR_480P_FPS_FIXED_0		15000
#define CONFIG_SENSOR_SVGA_FPS_FIXED_0		15000
#define CONFIG_SENSOR_720P_FPS_FIXED_0		30000

#define CONFIG_SENSOR_01  RK29_CAM_SENSOR_OV5642                   /* back camera sensor 1 */
#define CONFIG_SENSOR_IIC_ADDR_01 	    0x00
#define CONFIG_SENSOR_CIF_INDEX_01                    0
#define CONFIG_SENSOR_IIC_ADAPTER_ID_01    4
#define CONFIG_SENSOR_ORIENTATION_01       90
#define CONFIG_SENSOR_POWER_PIN_01         INVALID_GPIO
#define CONFIG_SENSOR_RESET_PIN_01         INVALID_GPIO
#define CONFIG_SENSOR_POWERDN_PIN_01       RK2928_PIN3_PB3
#define CONFIG_SENSOR_FALSH_PIN_01         INVALID_GPIO
#define CONFIG_SENSOR_POWERACTIVE_LEVEL_01 RK29_CAM_POWERACTIVE_L
#define CONFIG_SENSOR_RESETACTIVE_LEVEL_01 RK29_CAM_RESETACTIVE_L
#define CONFIG_SENSOR_POWERDNACTIVE_LEVEL_01 RK29_CAM_POWERDNACTIVE_H
#define CONFIG_SENSOR_FLASHACTIVE_LEVEL_01 RK29_CAM_FLASHACTIVE_L

#define CONFIG_SENSOR_QCIF_FPS_FIXED_01      15000
#define CONFIG_SENSOR_240X160_FPS_FIXED_01   15000
#define CONFIG_SENSOR_QVGA_FPS_FIXED_01      15000
#define CONFIG_SENSOR_CIF_FPS_FIXED_01       15000
#define CONFIG_SENSOR_VGA_FPS_FIXED_01       15000
#define CONFIG_SENSOR_480P_FPS_FIXED_01      15000
#define CONFIG_SENSOR_SVGA_FPS_FIXED_01      15000
#define CONFIG_SENSOR_720P_FPS_FIXED_01     30000

#define CONFIG_SENSOR_02 RK29_CAM_SENSOR_OV5640                      /* back camera sensor 2 */
#define CONFIG_SENSOR_IIC_ADDR_02 	    0x00
#define CONFIG_SENSOR_CIF_INDEX_02                    0
#define CONFIG_SENSOR_IIC_ADAPTER_ID_02    4
#define CONFIG_SENSOR_ORIENTATION_02       90
#define CONFIG_SENSOR_POWER_PIN_02         INVALID_GPIO
#define CONFIG_SENSOR_RESET_PIN_02         INVALID_GPIO
#define CONFIG_SENSOR_POWERDN_PIN_02       RK2928_PIN3_PB3
#define CONFIG_SENSOR_FALSH_PIN_02         INVALID_GPIO
#define CONFIG_SENSOR_POWERACTIVE_LEVEL_02 RK29_CAM_POWERACTIVE_L
#define CONFIG_SENSOR_RESETACTIVE_LEVEL_02 RK29_CAM_RESETACTIVE_L
#define CONFIG_SENSOR_POWERDNACTIVE_LEVEL_02 RK29_CAM_POWERDNACTIVE_H
#define CONFIG_SENSOR_FLASHACTIVE_LEVEL_02 RK29_CAM_FLASHACTIVE_L

#define CONFIG_SENSOR_QCIF_FPS_FIXED_02      15000
#define CONFIG_SENSOR_240X160_FPS_FIXED_02   15000
#define CONFIG_SENSOR_QVGA_FPS_FIXED_02      15000
#define CONFIG_SENSOR_CIF_FPS_FIXED_02       15000
#define CONFIG_SENSOR_VGA_FPS_FIXED_02       15000
#define CONFIG_SENSOR_480P_FPS_FIXED_02      15000
#define CONFIG_SENSOR_SVGA_FPS_FIXED_02      15000
#define CONFIG_SENSOR_720P_FPS_FIXED_02      30000

#define CONFIG_SENSOR_1 RK29_CAM_SENSOR_GC0308                      /* front camera sensor 0 */
#define CONFIG_SENSOR_IIC_ADDR_1 	    0x00
#define CONFIG_SENSOR_IIC_ADAPTER_ID_1	  1
#define CONFIG_SENSOR_CIF_INDEX_1				  0
#define CONFIG_SENSOR_ORIENTATION_1       270
#define CONFIG_SENSOR_POWER_PIN_1         INVALID_GPIO
#define CONFIG_SENSOR_RESET_PIN_1         INVALID_GPIO
#define CONFIG_SENSOR_POWERDN_PIN_1 	  RK30_PIN3_PD7
#define CONFIG_SENSOR_FALSH_PIN_1         INVALID_GPIO
#define CONFIG_SENSOR_POWERACTIVE_LEVEL_1 RK29_CAM_POWERACTIVE_L
#define CONFIG_SENSOR_RESETACTIVE_LEVEL_1 RK29_CAM_RESETACTIVE_L
#define CONFIG_SENSOR_POWERDNACTIVE_LEVEL_1 RK29_CAM_POWERDNACTIVE_H
#define CONFIG_SENSOR_FLASHACTIVE_LEVEL_1 RK29_CAM_FLASHACTIVE_L

#define CONFIG_SENSOR_QCIF_FPS_FIXED_1		15000
#define CONFIG_SENSOR_240X160_FPS_FIXED_1   15000
#define CONFIG_SENSOR_QVGA_FPS_FIXED_1		15000
#define CONFIG_SENSOR_CIF_FPS_FIXED_1		15000
#define CONFIG_SENSOR_VGA_FPS_FIXED_1		15000
#define CONFIG_SENSOR_480P_FPS_FIXED_1		15000
#define CONFIG_SENSOR_SVGA_FPS_FIXED_1		15000
#define CONFIG_SENSOR_720P_FPS_FIXED_1		30000

#define CONFIG_SENSOR_11 RK29_CAM_SENSOR_OV2659                      /* front camera sensor 1 */
#define CONFIG_SENSOR_IIC_ADDR_11 	    0x00
#define CONFIG_SENSOR_IIC_ADAPTER_ID_11    3
#define CONFIG_SENSOR_CIF_INDEX_11				  0
#define CONFIG_SENSOR_ORIENTATION_11       270
#define CONFIG_SENSOR_POWER_PIN_11         INVALID_GPIO
#define CONFIG_SENSOR_RESET_PIN_11         INVALID_GPIO
#define CONFIG_SENSOR_POWERDN_PIN_11       INVALID_GPIO
#define CONFIG_SENSOR_FALSH_PIN_11         INVALID_GPIO
#define CONFIG_SENSOR_POWERACTIVE_LEVEL_11 RK29_CAM_POWERACTIVE_L
#define CONFIG_SENSOR_RESETACTIVE_LEVEL_11 RK29_CAM_RESETACTIVE_L
#define CONFIG_SENSOR_POWERDNACTIVE_LEVEL_11 RK29_CAM_POWERDNACTIVE_H
#define CONFIG_SENSOR_FLASHACTIVE_LEVEL_11 RK29_CAM_FLASHACTIVE_L

#define CONFIG_SENSOR_QCIF_FPS_FIXED_11      15000
#define CONFIG_SENSOR_240X160_FPS_FIXED_11   15000
#define CONFIG_SENSOR_QVGA_FPS_FIXED_11      15000
#define CONFIG_SENSOR_CIF_FPS_FIXED_11       15000
#define CONFIG_SENSOR_VGA_FPS_FIXED_11       15000
#define CONFIG_SENSOR_480P_FPS_FIXED_11      15000
#define CONFIG_SENSOR_SVGA_FPS_FIXED_11      15000
#define CONFIG_SENSOR_720P_FPS_FIXED_11      30000

#define CONFIG_SENSOR_12 RK29_CAM_SENSOR_OV2659//RK29_CAM_SENSOR_OV2655                      /* front camera sensor 2 */
#define CONFIG_SENSOR_IIC_ADDR_12 	   0x00
#define CONFIG_SENSOR_IIC_ADAPTER_ID_12    3
#define CONFIG_SENSOR_CIF_INDEX_12				  0
#define CONFIG_SENSOR_ORIENTATION_12       270
#define CONFIG_SENSOR_POWER_PIN_12         INVALID_GPIO
#define CONFIG_SENSOR_RESET_PIN_12         INVALID_GPIO
#define CONFIG_SENSOR_POWERDN_PIN_12       INVALID_GPIO
#define CONFIG_SENSOR_FALSH_PIN_12         INVALID_GPIO
#define CONFIG_SENSOR_POWERACTIVE_LEVEL_12 RK29_CAM_POWERACTIVE_L
#define CONFIG_SENSOR_RESETACTIVE_LEVEL_12 RK29_CAM_RESETACTIVE_L
#define CONFIG_SENSOR_POWERDNACTIVE_LEVEL_12 RK29_CAM_POWERDNACTIVE_H
#define CONFIG_SENSOR_FLASHACTIVE_LEVEL_12 RK29_CAM_FLASHACTIVE_L

#define CONFIG_SENSOR_QCIF_FPS_FIXED_12      15000
#define CONFIG_SENSOR_240X160_FPS_FIXED_12   15000
#define CONFIG_SENSOR_QVGA_FPS_FIXED_12      15000
#define CONFIG_SENSOR_CIF_FPS_FIXED_12       15000
#define CONFIG_SENSOR_VGA_FPS_FIXED_12       15000
#define CONFIG_SENSOR_480P_FPS_FIXED_12      15000
#define CONFIG_SENSOR_SVGA_FPS_FIXED_12      15000
#define CONFIG_SENSOR_720P_FPS_FIXED_12      30000


#endif  //#ifdef CONFIG_VIDEO_RK29
/*---------------- Camera Sensor Configuration Macro End------------------------*/
#include "../../../drivers/media/video/rk2928_camera.c"
/*---------------- Camera Sensor Macro Define End  ---------*/

#define PMEM_CAM_SIZE PMEM_CAM_NECESSARY
/*****************************************************************************************
 * camera  devices
 * author: ddl@rock-chips.com
 *****************************************************************************************/
#ifdef CONFIG_VIDEO_RK29
#if 0
#define CONFIG_SENSOR_POWER_IOCTL_USR	   0 //define this refer to your board layout
#define CONFIG_SENSOR_RESET_IOCTL_USR	   0
#define CONFIG_SENSOR_POWERDOWN_IOCTL_USR	   0
#define CONFIG_SENSOR_FLASH_IOCTL_USR	   0
#else
#if defined(CONFIG_MACH_RK3026_PHONEPAD_780)
#define CAMERA_NAME                        	"gc2035_back_3"
#else
#ifdef CONFIG_SOC_CAMERA_SP0838		
#define CAMERA_NAME                      	"sp0838_back_3"
#elif defined( CONFIG_SOC_CAMERA_GC2035 )
#define CAMERA_NAME                        	"gc2035_back_3"
#elif defined( CONFIG_SOC_CAMERA_SP0A19 )
#define CAMERA_NAME                      	"sp0a19_back_3"
#elif defined( CONFIG_SOC_CAMERA_GC0329 )
#define CAMERA_NAME                      	"gc0329_back_3"
#elif defined( CONFIG_SOC_CAMERA_GC0309 )
#define CAMERA_NAME                      	"gc0309_back_3"
#else
#define CAMERA_NAME                        	"gc2035_back_3"
#endif
#endif
#define CONFIG_SENSOR_POWER_IOCTL_USR	   1 //define this refer to your board layout
#define CONFIG_SENSOR_RESET_IOCTL_USR	   0
#define CONFIG_SENSOR_POWERDOWN_IOCTL_USR  1 	    
#define CONFIG_SENSOR_FLASH_IOCTL_USR	   0
#define CONFIG_SENSOR_POWERDNACTIVE_LEVEL_PMU  1

#endif
static void rk_cif_power(int on)
{
    struct regulator *ldo_18,*ldo_28;

#if  defined(CONFIG_PWM_CONTROL_LOGIC) && defined(CONFIG_PWM_CONTROL_ARM)
#else
#if defined(CONFIG_MFD_TPS65910)
	if(pmic_is_tps65910())
	{
	    ldo_28 = regulator_get(NULL, "vaux1");   // vcc28_cif
	    ldo_18 = regulator_get(NULL, "vdig1");   // vcc18_cif
	}
#endif
	
#if  defined(CONFIG_KP_AXP20)
	if(pmic_is_axp202())
	{
	   ldo_28 = regulator_get(NULL, "ldo2");   // vcc28_cif
	   ldo_18 = regulator_get(NULL, "ldo3");   // vcc18_cif
	}
#endif

#if  defined(CONFIG_KP_AXP19)
	if(pmic_is_axp202())
	{
	   ldo_28 = regulator_get(NULL, "ldoio0");   // vcc28_cif
	   ldo_18 = ldo_28;
	}
#endif

#if  defined(CONFIG_REGULATOR_ACT8931)
	if(pmic_is_act8931())
	{
	   ldo_28 = regulator_get(NULL, "act_ldo1");   // vcc28_cif
	   ldo_18 = regulator_get(NULL, "act_ldo2");   // vcc18_cif
	}
#endif
#endif

	if (ldo_28 == NULL || IS_ERR(ldo_28) || ldo_18 == NULL || IS_ERR(ldo_18)){
     	   printk("get cif ldo failed!\n");
		return;
	    }
    if(on == 0){
#ifndef CONFIG_SOC_CAMERA_SP0838		
			while(regulator_is_enabled(ldo_28))	
    			regulator_disable(ldo_28);
    			regulator_put(ldo_28);

#ifndef CONFIG_KP_AXP19
			while(regulator_is_enabled(ldo_18))	
	    		regulator_disable(ldo_18);
    			regulator_put(ldo_18);
#endif
    	mdelay(500);
#endif

        }
    else{
    	regulator_set_voltage(ldo_28, 2800000, 2800000);
    	regulator_enable(ldo_28);
   // 	printk("%s set ldo7 vcc28_cif=%dmV end\n", __func__, regulator_get_voltage(ldo_28));
    	regulator_put(ldo_28);

#ifndef CONFIG_KP_AXP19
    	regulator_set_voltage(ldo_18, 1800000, 1800000);
    //	regulator_set_suspend_voltage(ldo, 1800000);
    	regulator_enable(ldo_18);
    //	printk("%s set ldo1 vcc18_cif=%dmV end\n", __func__, regulator_get_voltage(ldo_18));
    	regulator_put(ldo_18);
#endif
        }
}

#if CONFIG_SENSOR_POWER_IOCTL_USR
static int sensor_power_usr_cb (struct rk29camera_gpio_res *res,int on)
{
	    //#error "CONFIG_SENSOR_POWER_IOCTL_USR is 1, sensor_power_usr_cb function must be writed!!";
	struct regulator *ldo_18,*ldo_28;

#if  defined(CONFIG_PWM_CONTROL_LOGIC) && defined(CONFIG_PWM_CONTROL_ARM)
#else
#if defined(CONFIG_MFD_TPS65910)
	if(pmic_is_tps65910())
	{
	    ldo_28 = regulator_get(NULL, "vaux1");   // vcc28_cif
	    ldo_18 = regulator_get(NULL, "vdig1");   // vcc18_cif
	}
#endif

#if  defined(CONFIG_KP_AXP20)
	if(pmic_is_axp202())
	{
	   ldo_28 = regulator_get(NULL, "ldo2");   // vcc28_cif
	   ldo_18 = regulator_get(NULL, "ldo3");   // vcc18_cif
	}
#endif

#if  defined(CONFIG_KP_AXP19)
	if(pmic_is_axp202())
	{
	   ldo_28 = regulator_get(NULL, "ldoio0");   // vcc28_cif
	   ldo_18 = ldo_28;
	}
#endif

#if  defined(CONFIG_REGULATOR_ACT8931)
	if(pmic_is_act8931())
	{
	   ldo_28 = regulator_get(NULL, "act_ldo1");   // vcc28_cif
	   ldo_18 = regulator_get(NULL, "act_ldo2");   // vcc18_cif
	}
#endif
#endif

	if (ldo_28 == NULL || IS_ERR(ldo_28) || ldo_18 == NULL || IS_ERR(ldo_18)){
	    printk("get cif ldo failed!\n");
	    return -1;
	}
	printk("====liufeng 20131101:%s,%s,%d, %d\n\n",__func__,res->dev_name,on, strstr(res->dev_name, "sp0838"));
	if(on == 0){
#ifdef CONFIG_SOC_CAMERA_SP0838		
		if(strstr(res->dev_name, "sp0838")==0)
#endif
		{
			printk("====liufeng 20131101 disable ldo_28 =====\n");
		#if defined(CONFIG_MFD_TRS65910)
			regulator_set_voltage(ldo_28, 2800000, 2800000);
			regulator_enable(ldo_28);
			udelay(500);
		#else
		    	while(regulator_is_enabled(ldo_28)>0)
		#endif
				regulator_disable(ldo_28);
		    	regulator_put(ldo_28);

#ifndef CONFIG_KP_AXP19
		#if defined(CONFIG_MFD_TRS65910)
			regulator_set_voltage(ldo_18, 1800000, 1800000);
			regulator_enable(ldo_18);
			udelay(500);
		#else
		    	while(regulator_is_enabled(ldo_18)>0)
		#endif
				regulator_disable(ldo_18);
		    	regulator_put(ldo_18);
#endif

	    		mdelay(10);
		}
	} else {
	    regulator_set_voltage(ldo_28, 2800000, 2800000);
	    regulator_enable(ldo_28);
	    //printk("%s set ldo7 vcc28_cif=%dmV end\n", __func__, regulator_get_voltage(ldo_28));
	    regulator_put(ldo_28);
#ifndef CONFIG_KP_AXP19	
	    regulator_set_voltage(ldo_18, 1800000, 1800000);
	    //regulator_set_suspend_voltage(ldo, 1800000);
	    regulator_enable(ldo_18);
	    //printk("%s set ldo1 vcc18_cif=%dmV end\n", __func__, regulator_get_voltage(ldo_18));
	    regulator_put(ldo_18);
#endif
	}
	
	return 0;
}
#endif

#if CONFIG_SENSOR_RESET_IOCTL_USR
static int sensor_reset_usr_cb (struct rk29camera_gpio_res *res,int on)
{
	#error "CONFIG_SENSOR_RESET_IOCTL_USR is 1, sensor_reset_usr_cb function must be writed!!";
}
#endif

#if CONFIG_SENSOR_POWERDOWN_IOCTL_USR
static void rk_cif_powerdowen(int on)
{
    struct regulator *ldo_28;
    ldo_28 = regulator_get(NULL, "vpll");   // vcc28_cif
    if (ldo_28 == NULL || IS_ERR(ldo_28) ){
            printk("get cif vpll ldo failed!\n");
            return;
    }
    
  //  if((res->gpio_flag & RK29_CAM_POWERDNACTIVE_MASK) == RK29_CAM_POWERDNACTIVE_H) {
      if( CONFIG_SENSOR_POWERDNACTIVE_LEVEL_PMU ) {
		printk("hjc:%s[%d],on=%d\n",__func__,__LINE__,on);
		if(on == 0){//enable camera
		       regulator_set_voltage(ldo_28, 2500000, 2500000);
		       regulator_enable(ldo_28);
		       printk(" %s set  vpll vcc28_cif=%dmV end\n", __func__, regulator_get_voltage(ldo_28));
		       regulator_put(ldo_28);

		} else {//disable camera
			if(regulator_is_enabled(ldo_28)>0){
				printk("%s[%d]\n",__func__,__LINE__);
				int a = regulator_disable(ldo_28);
			}
			regulator_put(ldo_28);
			mdelay(500);
		}
     }else{
           	printk("hjc:%s[%d],on=%d\n",__func__,__LINE__,on);
            
		if(on == 1){//enable camera
		       regulator_set_voltage(ldo_28, 2500000, 2500000);
		       while(!regulator_is_enabled(ldo_28))
			   regulator_enable(ldo_28);
		       printk(" %s set  vpll vcc28_cif=%dmV end\n", __func__, regulator_get_voltage(ldo_28));
		       regulator_put(ldo_28);
		}else{//disable camera
				if(regulator_is_enabled(ldo_28)>0){
					regulator_disable(ldo_28);
			}
			regulator_put(ldo_28);
			mdelay(500);
		}
     }
}
static int sensor_powerdown_usr_cb (struct rk29camera_gpio_res *res,int on)
{
int camera_powerdown = res->gpio_powerdown;

    #if 1 //defined(CONFIG_MACH_RK2926_V86)
    int ret = 0; 
    printk("hjc:%s,%s,on=%d\n\n\n",__func__,res->dev_name,on);
#ifdef CONFIG_CAMERA_PMU_GPIO_RESEVRT	
    if(strcmp(res->dev_name,CAMERA_NAME)==0 )
    { //gpio\BF\D8\D6ƵĲ\D9\D7\F7
         //   int camera_powerdown = res->gpio_powerdown;
            int camera_ioflag = res->gpio_flag;
            int camera_io_init = res->gpio_init; //  int ret = 0;    
            if (camera_powerdown != INVALID_GPIO) { 
                    if (camera_io_init & RK29_CAM_POWERDNACTIVE_MASK) {
                          if (on) {
                                gpio_set_value(camera_powerdown, ((camera_ioflag&RK29_CAM_POWERDNACTIVE_MASK)>>RK29_CAM_POWERDNACTIVE_BITPOS)); 
                                printk("%s..%s..PowerDownPin=%d ..PinLevel = %x \n",__FUNCTION__,res->dev_name,camera_powerdown, ((camera_ioflag&RK29_CAM_POWERDNACTIVE_MASK)>>RK29_CAM_POWERDNACTIVE_BITPOS)); 
                         } else { 
                                gpio_set_value(camera_powerdown,(((~camera_ioflag)&RK29_CAM_POWERDNACTIVE_MASK)>>RK29_CAM_POWERDNACTIVE_BITPOS)); 
                                printk("%s..%s..PowerDownPin= %d..PinLevel = %x   \n",__FUNCTION__,res->dev_name, camera_powerdown, (((~camera_ioflag)&RK29_CAM_POWERDNACTIVE_MASK)>>RK29_CAM_POWERDNACTIVE_BITPOS)); 
                         }
                  } else { ret = RK29_CAM_EIO_REQUESTFAIL;
                                printk("%s..%s..PowerDownPin=%d request failed!\n",__FUNCTION__,res->dev_name,camera_powerdown); } 
          } else { 
                ret = RK29_CAM_EIO_INVALID; 
          }
    }
	else
    { 
        printk("\n\n%s.............pwm power,on=%d\n",__FUNCTION__,on);
        rk_cif_powerdowen(on);
    }
    return ret;
#else
    	if(strcmp(res->dev_name,CAMERA_NAME)==0 )
	{ 
		//\C8\E7\B9\FBΪpmu\BF\D8\D6Ƶ\C4\D2\FD\BDţ\AC"ov5642_front_1" \B8\F9\BE\DD sensor\C3\FB\D7\D6 \A3\ACǰ\BA\F3\D6\C3 \A3\AC sensor\D0\F2\BA\C5ȷ\B6\A8 
		//\BE\DF\CC\E5pmu\BF\D8\D6Ʋ\D9\D7\F7\A3\AC\BFɲο\BC\CEļ\FEĩβ\B5Ĳο\BC\B4\FA\C2\EB 
		printk("\n\n%s.............pwm power,on=%d\n",__FUNCTION__,on);
		rk_cif_powerdowen(on);
	}else{ //gpio\BF\D8\D6ƵĲ\D9\D7\F7
		 //   int camera_powerdown = res->gpio_powerdown;
		int camera_ioflag = res->gpio_flag;
		int camera_io_init = res->gpio_init; //  int ret = 0;	 
		if (camera_powerdown != INVALID_GPIO) { 
			if (camera_io_init & RK29_CAM_POWERDNACTIVE_MASK) {
				if (on) {
					gpio_set_value(camera_powerdown, ((camera_ioflag&RK29_CAM_POWERDNACTIVE_MASK)>>RK29_CAM_POWERDNACTIVE_BITPOS)); 
					printk("%s..%s..PowerDownPin=%d ..PinLevel = %x \n",__FUNCTION__,res->dev_name,camera_powerdown, ((camera_ioflag&RK29_CAM_POWERDNACTIVE_MASK)>>RK29_CAM_POWERDNACTIVE_BITPOS)); 
				 } else { 
					gpio_set_value(camera_powerdown,(((~camera_ioflag)&RK29_CAM_POWERDNACTIVE_MASK)>>RK29_CAM_POWERDNACTIVE_BITPOS)); 
					printk("%s..%s..PowerDownPin= %d..PinLevel = %x   \n",__FUNCTION__,res->dev_name, camera_powerdown, (((~camera_ioflag)&RK29_CAM_POWERDNACTIVE_MASK)>>RK29_CAM_POWERDNACTIVE_BITPOS)); 
				 }
			} else { 
				ret = RK29_CAM_EIO_REQUESTFAIL;
				printk("%s..%s..PowerDownPin=%d request failed!\n",__FUNCTION__,res->dev_name,camera_powerdown); 
			} 
		} else { 
			ret = RK29_CAM_EIO_INVALID; 
		}
	}
	return ret;
#endif

    #else
    #error "CONFIG_SENSOR_POWERDOWN_IOCTL_USR is 1, sensor_powerdown_usr_cb function must be writed!!";
    #endif
}
#endif

#if CONFIG_SENSOR_FLASH_IOCTL_USR
static int sensor_flash_usr_cb (struct rk29camera_gpio_res *res,int on)
{
	#error "CONFIG_SENSOR_FLASH_IOCTL_USR is 1, sensor_flash_usr_cb function must be writed!!";
}
#endif

static struct rk29camera_platform_ioctl_cb	sensor_ioctl_cb = {
	#if CONFIG_SENSOR_POWER_IOCTL_USR
	.sensor_power_cb = sensor_power_usr_cb,
	#else
	.sensor_power_cb = NULL,
	#endif

	#if CONFIG_SENSOR_RESET_IOCTL_USR
	.sensor_reset_cb = sensor_reset_usr_cb,
	#else
	.sensor_reset_cb = NULL,
	#endif

	#if CONFIG_SENSOR_POWERDOWN_IOCTL_USR
	.sensor_powerdown_cb = sensor_powerdown_usr_cb,
	#else
	.sensor_powerdown_cb = NULL,
	#endif

	#if CONFIG_SENSOR_FLASH_IOCTL_USR
	.sensor_flash_cb = sensor_flash_usr_cb,
	#else
	.sensor_flash_cb = NULL,
	#endif
};

#if CONFIG_SENSOR_IIC_ADDR_0
static struct reginfo_t rk_init_data_sensor_reg_0[] =
{
		{0x0000, 0x00,0,0}
	};
static struct reginfo_t rk_init_data_sensor_winseqreg_0[] ={
	{0x0000, 0x00,0,0}
	};
#endif

#if CONFIG_SENSOR_IIC_ADDR_1
static struct reginfo_t rk_init_data_sensor_reg_1[] =
{
    {0x0000, 0x00,0,0}
};
static struct reginfo_t rk_init_data_sensor_winseqreg_1[] =
{
       {0x0000, 0x00,0,0}
};
#endif
#if CONFIG_SENSOR_IIC_ADDR_01
static struct reginfo_t rk_init_data_sensor_reg_01[] =
{
    {0x0000, 0x00,0,0}
};
static struct reginfo_t rk_init_data_sensor_winseqreg_01[] =
{
       {0x0000, 0x00,0,0}
};
#endif
#if CONFIG_SENSOR_IIC_ADDR_02
static struct reginfo_t rk_init_data_sensor_reg_02[] =
{
    {0x0000, 0x00,0,0}
};
static struct reginfo_t rk_init_data_sensor_winseqreg_02[] =
{
       {0x0000, 0x00,0,0}
};
#endif
#if CONFIG_SENSOR_IIC_ADDR_11
static struct reginfo_t rk_init_data_sensor_reg_11[] =
{
    {0x0000, 0x00,0,0}
};
static struct reginfo_t rk_init_data_sensor_winseqreg_11[] =
{
       {0x0000, 0x00,0,0}
};
#endif
#if CONFIG_SENSOR_IIC_ADDR_12
static struct reginfo_t rk_init_data_sensor_reg_12[] =
{
    {0x0000, 0x00,0,0}
};
static struct reginfo_t rk_init_data_sensor_winseqreg_12[] =
{
       {0x0000, 0x00,0,0}
};
#endif
static rk_sensor_user_init_data_s rk_init_data_sensor[RK_CAM_NUM] = 
{
    #if CONFIG_SENSOR_IIC_ADDR_0
    {
       .rk_sensor_init_width = INVALID_VALUE,
       .rk_sensor_init_height = INVALID_VALUE,
       .rk_sensor_init_bus_param = INVALID_VALUE,
       .rk_sensor_init_pixelcode = INVALID_VALUE,
       .rk_sensor_init_data = rk_init_data_sensor_reg_0,
       .rk_sensor_init_winseq = rk_init_data_sensor_winseqreg_0,
       .rk_sensor_winseq_size = sizeof(rk_init_data_sensor_winseqreg_0) / sizeof(struct reginfo_t),
       .rk_sensor_init_data_size = sizeof(rk_init_data_sensor_reg_0) / sizeof(struct reginfo_t),
    },
    #else
    {
       .rk_sensor_init_width = INVALID_VALUE,
       .rk_sensor_init_height = INVALID_VALUE,
       .rk_sensor_init_bus_param = INVALID_VALUE,
       .rk_sensor_init_pixelcode = INVALID_VALUE,
       .rk_sensor_init_data = NULL,
       .rk_sensor_init_winseq = NULL,
       .rk_sensor_winseq_size = 0,
       .rk_sensor_init_data_size = 0,
    },
    #endif
    #if CONFIG_SENSOR_IIC_ADDR_1
    {
       .rk_sensor_init_width = INVALID_VALUE,
       .rk_sensor_init_height = INVALID_VALUE,
       .rk_sensor_init_bus_param = INVALID_VALUE,
       .rk_sensor_init_pixelcode = INVALID_VALUE,
       .rk_sensor_init_data = rk_init_data_sensor_reg_1,
       .rk_sensor_init_winseq = rk_init_data_sensor_winseqreg_1,
       .rk_sensor_winseq_size = sizeof(rk_init_data_sensor_winseqreg_1) / sizeof(struct reginfo_t),
       .rk_sensor_init_data_size = sizeof(rk_init_data_sensor_reg_1) / sizeof(struct reginfo_t),
    },
    #else
    {
       .rk_sensor_init_width = INVALID_VALUE,
       .rk_sensor_init_height = INVALID_VALUE,
       .rk_sensor_init_bus_param = INVALID_VALUE,
       .rk_sensor_init_pixelcode = INVALID_VALUE,
       .rk_sensor_init_data = NULL,
       .rk_sensor_init_winseq = NULL,
       .rk_sensor_winseq_size = 0,
       .rk_sensor_init_data_size = 0,
    },
    #endif
    #if CONFIG_SENSOR_IIC_ADDR_01
    {
       .rk_sensor_init_width = INVALID_VALUE,
       .rk_sensor_init_height = INVALID_VALUE,
       .rk_sensor_init_bus_param = INVALID_VALUE,
       .rk_sensor_init_pixelcode = INVALID_VALUE,
       .rk_sensor_init_data = rk_init_data_sensor_reg_01,
       .rk_sensor_init_winseq = rk_init_data_sensor_winseqreg_01,
       .rk_sensor_winseq_size = sizeof(rk_init_data_sensor_winseqreg_01) / sizeof(struct reginfo_t),
       .rk_sensor_init_data_size = sizeof(rk_init_data_sensor_reg_01) / sizeof(struct reginfo_t),
    },
    #else
    {
       .rk_sensor_init_width = INVALID_VALUE,
       .rk_sensor_init_height = INVALID_VALUE,
       .rk_sensor_init_bus_param = INVALID_VALUE,
       .rk_sensor_init_pixelcode = INVALID_VALUE,
       .rk_sensor_init_data = NULL,
       .rk_sensor_init_winseq = NULL,
       .rk_sensor_winseq_size = 0,
       .rk_sensor_init_data_size = 0,
    },
    #endif
    #if CONFIG_SENSOR_IIC_ADDR_02
    {
       .rk_sensor_init_width = INVALID_VALUE,
       .rk_sensor_init_height = INVALID_VALUE,
       .rk_sensor_init_bus_param = INVALID_VALUE,
       .rk_sensor_init_pixelcode = INVALID_VALUE,
       .rk_sensor_init_data = rk_init_data_sensor_reg_02,
       .rk_sensor_init_winseq = rk_init_data_sensor_winseqreg_02,
       .rk_sensor_winseq_size = sizeof(rk_init_data_sensor_winseqreg_02) / sizeof(struct reginfo_t),
       .rk_sensor_init_data_size = sizeof(rk_init_data_sensor_reg_02) / sizeof(struct reginfo_t),
    },
    #else
    {
       .rk_sensor_init_width = INVALID_VALUE,
       .rk_sensor_init_height = INVALID_VALUE,
       .rk_sensor_init_bus_param = INVALID_VALUE,
       .rk_sensor_init_pixelcode = INVALID_VALUE,
       .rk_sensor_init_data = NULL,
       .rk_sensor_init_winseq = NULL,
       .rk_sensor_winseq_size = 0,
       .rk_sensor_init_data_size = 0,
    },
    #endif
    #if CONFIG_SENSOR_IIC_ADDR_11
    {
       .rk_sensor_init_width = INVALID_VALUE,
       .rk_sensor_init_height = INVALID_VALUE,
       .rk_sensor_init_bus_param = INVALID_VALUE,
       .rk_sensor_init_pixelcode = INVALID_VALUE,
       .rk_sensor_init_data = rk_init_data_sensor_reg_11,
       .rk_sensor_init_winseq = rk_init_data_sensor_winseqreg_11,
       .rk_sensor_winseq_size = sizeof(rk_init_data_sensor_winseqreg_11) / sizeof(struct reginfo_t),
       .rk_sensor_init_data_size = sizeof(rk_init_data_sensor_reg_11) / sizeof(struct reginfo_t),
    },
    #else
    {
       .rk_sensor_init_width = INVALID_VALUE,
       .rk_sensor_init_height = INVALID_VALUE,
       .rk_sensor_init_bus_param = INVALID_VALUE,
       .rk_sensor_init_pixelcode = INVALID_VALUE,
       .rk_sensor_init_data = NULL,
       .rk_sensor_init_winseq = NULL,
       .rk_sensor_winseq_size = 0,
       .rk_sensor_init_data_size = 0,
    },
    #endif
    #if CONFIG_SENSOR_IIC_ADDR_12
    {
       .rk_sensor_init_width = INVALID_VALUE,
       .rk_sensor_init_height = INVALID_VALUE,
       .rk_sensor_init_bus_param = INVALID_VALUE,
       .rk_sensor_init_pixelcode = INVALID_VALUE,
       .rk_sensor_init_data = rk_init_data_sensor_reg_12,
       .rk_sensor_init_winseq = rk_init_data_sensor_winseqreg_12,
       .rk_sensor_winseq_size = sizeof(rk_init_data_sensor_winseqreg_12) / sizeof(struct reginfo_t),
       .rk_sensor_init_data_size = sizeof(rk_init_data_sensor_reg_12) / sizeof(struct reginfo_t),
    },
    #else
    {
       .rk_sensor_init_width = INVALID_VALUE,
       .rk_sensor_init_height = INVALID_VALUE,
       .rk_sensor_init_bus_param = INVALID_VALUE,
       .rk_sensor_init_pixelcode = INVALID_VALUE,
       .rk_sensor_init_data = NULL,
       .rk_sensor_init_winseq = NULL,
       .rk_sensor_winseq_size = 0,
       .rk_sensor_init_data_size = 0,
    },
    #endif

 };
#include "../../../drivers/media/video/rk2928_camera.c"

#endif /* CONFIG_VIDEO_RK29 */
