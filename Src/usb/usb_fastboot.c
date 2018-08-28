/*
 * (C) Copyright 2007-2013
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Jerry Wang <wangflord@allwinnertech.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#include "stdio.h"
#include "string.h"
#include "usb_base.h"
#include "scsi.h"
#include "dma.h"
//#include "sys_partition.h"
//#include "fastboot.h"
#include "usb_fastboot.h"
//#include <android_misc.h>
#include "sunxi_board.h"
#include "config.h"
#include "timer.h"
#include "stdlib.h"
#include "sun8iw7p1.h"

 

//DECLARE_GLOBAL_DATA_PTR;
 

#define FASTBOOT_INTERFACE_CLASS     0xff
#define FASTBOOT_INTERFACE_SUB_CLASS 0x42
#define FASTBOOT_INTERFACE_PROTOCOL  0x03

static int sunxi_usb_fastboot_write_enable = 0;
static int sunxi_usb_fastboot_status = SUNXI_USB_FASTBOOT_IDLE;

#define CONFIG_SYS_SDRAM_BASE		     (0x40000000)
#define CONFIG_SYS_TEXT_BASE		     (0x4A000000)
#define CONFIG_SYS_OBLIGATE_BASE         (0xF0000000)

#define FASTBOOT_TRANSFER_BUFFER		(CONFIG_SYS_SDRAM_BASE + 0x01000000)
#define FASTBOOT_TRANSFER_BUFFER_SIZE	(256 << 20)

#define FASTBOOT_ERASE_BUFFER			(CONFIG_SYS_SDRAM_BASE)
#define FASTBOOT_ERASE_BUFFER_SIZE      (16 << 20)

#define  MAX( x, y ) ( ((x) > (y)) ? (x) : (y) )
#define  MIN( x, y ) ( ((x) < (y)) ? (x) : (y) )

static fastboot_trans_set_t trans_data;

static uint all_download_bytes;

int fastboot_data_flag;

extern int sunxi_usb_exit(void);

int tick_printf(const char *fmt, ...)
{

    return 0;
}
/*
 *******************************************************************************
 *                     do_usb_req_set_interface
 *
 * Description:
 *    void
 *
 * Parameters:
 *    void
 *
 * Return value:
 *    void
 *
 * note:
 *    void
 *
 *******************************************************************************
 */
static int __usb_set_interface(struct usb_device_request *req)
{
	sunxi_usb_dbg("set interface\n");
	/* Only support interface 0, alternate 0 */
	if ((0 == req->wIndex) && (0 == req->wValue))
	{
		sunxi_udc_ep_reset();
	}
	else
	{
		printf("err: invalid wIndex and wValue, (0, %d), (0, %d)\n", req->wIndex, req->wValue);
		return SUNXI_USB_REQ_OP_ERR;
	}

    return SUNXI_USB_REQ_SUCCESSED;
}

/*
 *******************************************************************************
 *                     do_usb_req_set_address
 *
 * Description:
 *    void
 *
 * Parameters:
 *    void
 *
 * Return value:
 *    void
 *
 * note:
 *    void
 *
 *******************************************************************************
 */
static int __usb_set_address(struct usb_device_request *req)
{
    uchar address;

    address = req->wValue & 0x7f;
    sunxi_usb_dbg("set address 0x%x\n", address);

    sunxi_udc_set_address(address);

    return SUNXI_USB_REQ_SUCCESSED;
}

/*
 *******************************************************************************
 *                     do_usb_req_set_configuration
 *
 * Description:
 *    void
 *
 * Parameters:
 *    void
 *
 * Return value:
 *    void
 *
 * note:
 *    void
 *
 *******************************************************************************
 */
static int __usb_set_configuration(struct usb_device_request *req)
{
	sunxi_usb_dbg("set configuration = %d\n", req->wValue);
	/* Only support 1 configuration so nak anything else */
	if (1 == req->wValue)
	{
		sunxi_udc_ep_reset();
	}
	else
	{
		printf("err: invalid wValue, (0, %d)\n", req->wValue);

		return SUNXI_USB_REQ_OP_ERR; /// modify by wg
	}

	sunxi_udc_set_configuration(req->wValue);

	//sunxi_udc_send_setup(0, NULL); //add by wg

	return SUNXI_USB_REQ_SUCCESSED;
}
/*
 *******************************************************************************
 *                     do_usb_req_get_descriptor
 *
 * Description:
 *    void
 *
 * Parameters:
 *    void
 *
 * Return value:
 *    void
 *
 * note:
 *    void
 *
 *******************************************************************************
 */ 
 
#define MAX_TOUCH_POINT      10

#define REPORTID_STOUCH      1
#define REPORTID_MTOUCH      2
#define REPORTID_FEATURE     3

#if (USB_COMPLIE_MODE == USB_HID)
//--------单点触摸设备设备描述
uchar ReportDescriptor[] =
{
/***********************以下是单点触摸报告描述符********************/
//每行开始的第一字节为该条目的前缀，前缀的格式为：
//D7~D4：bTag。D3~D2：bType；D1~D0：bSize。以下分别对每个条目注释。
//这是一个全局（bType为1）条目，选择用途页为普通桌面Generic Desktop Page(0x01)
//后面跟一字节数据（bSize为1），后面的字节数就不注释了，
//自己根据bSize来判断。
        0x05,
        0x01, // USAGE_PAGE (Generic Desktop)

        //这是一个局部（bType为2）条目，说明接下来的应用集合用途用于鼠标
        0x09,
        0x02, // USAGE (Mouse)

        //这是一个主（bType为0）条目，开集合，后面跟的数据0x01表示
        //该集合是一个应用集合。它的性质在前面由用途页和用途定义为
        //普通桌面用的鼠标。
        0xa1,
        0x01, // COLLECTION (Application)

        //全局条目，定义单点触摸的报告ID为REPORTID_STOUCH(usbcore.h中定义)
        0x85,
        REPORTID_STOUCH, //   REPORT_ID (Single Touch)

        //这是一个局部条目。说明用途为指针集合
        0x09,
        0x01, //   USAGE (Pointer)

        //这是一个主条目，开集合，后面跟的数据0x00表示该集合是一个
        //物理集合，用途由前面的局部条目定义为指针集合。
        0xa1,
        0x00, //   COLLECTION (Physical)

        //这是一个全局条目，选择用途页为按键（Button Page(0x09)）
        0x05,
        0x09, //     USAGE_PAGE (Button)

        //这是一个局部条目，说明用途的最小值为1。实际上是鼠标左键。
        0x19,
        0x01, //     USAGE_MINIMUM (Button 1)

        //这是一个局部条目，说明用途的最大值为3。实际上是鼠标中键。
        0x29,
        0x03, //     USAGE_MAXIMUM (Button 3)

        //这是一个全局条目，说明返回的数据的逻辑值（就是我们返回的数据域的值啦）
        //最小为0。因为我们这里用Bit来表示一个数据域，因此最小为0，最大为1。
        0x15,
        0x00, //     LOGICAL_MINIMUM (0)

        //这是一个全局条目，说明逻辑值最大为1。
        0x25,
        0x01, //     LOGICAL_MAXIMUM (1)

        //这是一个全局条目，说明数据域的数量为三个。
        0x95,
        0x03, //     REPORT_COUNT (3)

        //这是一个全局条目，说明每个数据域的长度为1个bit。
        0x75,
        0x01, //     REPORT_SIZE (1)

        //这是一个主条目，说明有3个长度为1bit的数据域（数量和长度
        //由前面的两个全局条目所定义）用来做为输入，
        //属性为：Data,Var,Abs。Data表示这些数据可以变动，Var表示
        //这些数据域是独立的，每个域表示一个意思。Abs表示绝对值。
        //这样定义的结果就是，第一个数据域bit0表示按键1（左键）是否按下，
        //第二个数据域bit1表示按键2（右键）是否按下，第三个数据域bit2表示
        //按键3（中键）是否按下。
        0x81,
        0x02, //     INPUT (Data,Var,Abs)

        //这是一个全局条目，说明数据域数量为1个
        0x95,
        0x01, //     REPORT_COUNT (1)

        //这是一个全局条目，说明每个数据域的长度为5bit。
        0x75,
        0x05, //     REPORT_SIZE (5)

        //这是一个主条目，输入用，由前面两个全局条目可知，长度为5bit，
        //数量为1个。它的属性为常量（即返回的数据一直是0）。
        //这个只是为了凑齐一个字节（前面用了3个bit）而填充的一些数据
        //而已，所以它是没有实际用途的。
        0x81,
        0x03, //     INPUT (Cnst,Var,Abs)

        //这是一个全局条目，选择用途页为普通桌面Generic Desktop Page(0x01)
        0x05,
        0x01, //     USAGE_PAGE (Generic Desktop)

        //这是一个局部条目，说明用途为X轴
        0x09,
        0x30, //     USAGE (X)

        //这是一个局部条目，说明用途为Y轴
        0x09,
        0x31, //     USAGE (Y)

        //下面两个为全局条目，说明返回的逻辑最小和最大值。
        //这里定义X和Y轴的逻辑最小值为0，即坐标原点
        //X和Y轴的逻辑最大值为32768，即屏幕右下方的坐标为（32768，32768）。
        //由于32768超过了一字节的范围，所以需要用2字节的格式表示最大值
        0x15,
        0x00, //     LOGICAL_MINIMUM (0)
        0x26, 0xFF, 0x7F, //     LOGICAL_MAXIMUM (32768)

        //下面两个为全局条目，说明返回的物理最小和最大值。
        //这里定义X和Y轴的物理最小值为0，即坐标原点
        //X和Y轴的物理最大值为32768，即屏幕右下方的坐标为（32768，32768）。
        //由于4096超过了一字节的范围，所以需要用2字节的格式表示最大值
        0x35,
        0x00,    //Physical Minimum (0)
        0x46, 0xFF, 0x7F, //Physical Maximum(32768)

        //这是一个全局条目，说明数据域的长度为16bit。
        0x75, 0x10, //     REPORT_SIZE (16)

        //这是一个全局条目，说明数据域的个数为2个。
        0x95,
        0x02, //     REPORT_COUNT (2)

        //这是一个主条目。它说明这两个16bit的数据域是输入用的，
        //属性为：Data,Var,Abs。Data说明数据是可以变的，Var说明
        //这些数据域是独立的，Abs表示这些值是绝对值。
        0x81,
        0x02, //     INPUT (Data,Var,Abs)

        //下面这两个主条目用来关闭前面的集合用。
        //我们开了两个集合，所以要关两次。bSize为0，所以后面没数据。
        0xc0,//   END_COLLECTION
        0xc0,       // END_COLLECTION

        /***********************单点触摸描述完毕***************************/

        /***********************以下是多点触摸报告描述*********************/
        /*
         //这是一个全局条目，选择用途页为Digitizers
         0x05, 0x0d,  // USAGE_PAGE (Digitizers)

         //这是一个局部条目，说明接下来的应用集合用途用于Touch Screen
         0x09, 0x04,  // USAGE (Touch Screen)

         //这是一个主条目，开集合，后面跟的数据0x01表示该集合是一个应用集合。
         //它的性质在前面由用途页和用途定义为Digitizers的Touch Screen。
         0xa1, 0x01,  // COLLECTION (Application)

         //全局条目，定义多点触摸的报告ID为REPORTID_MTOUCH(usbcore.h中定义)
         0x85, REPORTID_MTOUCH,  //   REPORT_ID (Touch)

         //这是一个局部条目，选择用途为Digitizers页中的Finger
         0x09, 0x22,  //   USAGE (Finger)

         //这是一个主条目，开集合，后面跟的数据0x02表示该集合是一个逻辑集合。
         //它的性质在前面由用途页和用途定义为Digitizers的Finger。
         0xa1, 0x02,  //     COLLECTION (Logical)

         //这是一个局部条目，选择用途为Digitizers页中的Tip Switch，
         //本来用来表示触摸笔是否接触到屏幕，这里用来表示是否有手指触摸。
         0x09, 0x42,  //       USAGE (Tip Switch)

         //这是一个全局条目，说明返回的数据的逻辑最小值为0。
         0x15, 0x00,  //       LOGICAL_MINIMUM (0)

         //这是一个全局条目，说明返回的数据的逻辑最大值为1。
         0x25, 0x01,  //       LOGICAL_MAXIMUM (1)

         //这是一个全局条目，说明每个数据域的长度为1个bit。
         0x75, 0x01,  //       REPORT_SIZE (1)

         //这是一个全局条目，说明数据域的数量为1个。
         0x95, 0x01,  //       REPORT_COUNT (1)

         //这是一个主条目，说明有1个长度为1bit的数据域用于输入。
         //通过前面的描述我们可以知道，这个bit是用来表示是否触摸的，
         //0表示没有触摸，1表示触摸。
         0x81, 0x02,  //       INPUT (Data,Var,Abs)

         //这是一个局部条目，选择用途为Digitizers页中的In Range，
         //用来表示触摸的区域是否有效。
         0x09, 0x32,  //       USAGE (In Range)

         //这是一个主条目，说明有1个长度为1bit的数据域用于输入。
         //通过前面的描述我们可以知道，这个bit是用来表示触摸区域是否有效的，
         //0表示无效，1表示有效。
         0x81, 0x02,  //       INPUT (Data,Var,Abs)

         //这是一个局部条目，选择用途为Digitizers页中的Touch Valid，
         //用来表示触摸是否有效。
         0x09, 0x47,  //       USAGE (Touch Valid)

         //这是一个主条目，说明有1个长度为1bit的数据域用于输入。
         //通过前面的描述我们可以知道，这个bit是用来表示触摸是否有效的，
         //0表示无效，1表示有效。
         0x81, 0x02,  //       INPUT (Data,Var,Abs)

         //这是一个全局条目，说明数据域的数量为5个。
         0x95, 0x05,  //       REPORT_COUNT (5)

         //这是一个主条目，输入用，由前面两个全局条目可知，长度为1bit，
         //数量为5个。它的属性为常量（即返回的数据一直是0）。
         //这个只是为了凑齐一个字节（前面用了3个bit）而填充的一些数据
         //而已，所以它是没有实际用途的。
         0x81, 0x03,  //       INPUT (Cnst,Ary,Abs)

         //这是一个全局条目，说明返回的数据的逻辑最大值为MAX_TOUCH_POINT。
         0x25, MAX_TOUCH_POINT,  // LOGICAL_MAXIMUM (MAX_TOUCH_POINT)

         //这是一个全局条目，说明每个数据域的长度为8个bit。
         0x75, 0x08,  //       REPORT_SIZE (8)

         //这是一个局部条目，选择用途为Digitizers页中的Contact Identifier，
         //用来表示触摸的ID号。
         0x09, 0x51,  //       USAGE (Contact Identifier)

         //这是一个全局条目，说明数据域的数量为1个。
         0x95, 0x01,  //       REPORT_COUNT (1)

         //这是一个主条目，说明有1个长度为8bit的数据域用于输入。
         //通过前面的描述我们可以知道，这个8bit是用来表示触摸ID号的，
         0x81, 0x02,  //       INPUT (Data,Var,Abs)

         //这是一个全局条目，选择用途页为Generic Desktop
         0x05, 0x01,  //       USAGE_PAGE (Generic Desktop)

         //这是一个全局条目，说明返回的数据的逻辑最大值为4096。
         0x26, 0x00, 0x10,  //       LOGICAL_MAXIMUM (4096)

         //这是一个全局条目，说明每个数据域的长度为16个bit。
         0x75, 0x10,  //       REPORT_SIZE (16)

         //这是一个全局条目，说明物理单位的指数为0
         0x55, 0x00,  //       UNIT_EXPONENT (0)

         //这是一个全局条目，说明没有物理单位
         0x65, 0x00,  //       UNIT (None)

         //这是一个局部条目，选择用途为通用桌面中的X轴
         0x09, 0x30,  //       USAGE (X)

         //这是一个全局条目，说明物理最小值为0。由于这里用不到物理最小、
         //最大值，所以将物理最小、最大值描述成0，并将单位描述成无。
         0x35, 0x00,  //       PHYSICAL_MINIMUM (0)

         //这是一个全局条目，说明物理最大值为0。
         0x46, 0x00, 0x00,  //       PHYSICAL_MAXIMUM (0)

         //这是一个主条目。它说明这个16bit的数据域是输入用的，
         //属性为：Data,Var,Abs。Data说明数据是可以变的，Var说明
         //这些数据域是独立的，Abs表示这些值是绝对值。它其实就是
         //我们要返回的X坐标数据。
         0x81, 0x02,  //       INPUT (Data,Var,Abs)

         //这是一个局部条目，选择用途为通用桌面中的Y轴
         0x09, 0x31,  //       USAGE (Y)

         //这是一个全局条目，说明物理最大值为0。
         0x46, 0x00, 0x00,  //       PHYSICAL_MAXIMUM (0)

         //这是一个主条目。它说明这个16bit的数据域是输入用的，
         //属性为：Data,Var,Abs。Data说明数据是可以变的，Var说明
         //这些数据域是独立的，Abs表示这些值是绝对值。它其实就是
         //我们要返回的Y坐标数据。
         0x81, 0x02,  //       INPUT (Data,Var,Abs)

         //这是一个主条目，关闭前面开的逻辑集合
         0xc0,        //    END_COLLECTION

         //从这里一直到下面的END_COLLECTION，基本上是上一个逻辑集合的重复，
         //这里就不再给出注释了，请读者自行分析。
         0xa1, 0x02,  //    COLLECTION (Logical)
         0x05, 0x0d,  //     USAGE_PAGE (Digitizers)
         0x09, 0x42,  //       USAGE (Tip Switch)
         0x15, 0x00,  //       LOGICAL_MINIMUM (0)
         0x25, 0x01,  //       LOGICAL_MAXIMUM (1)
         0x75, 0x01,  //       REPORT_SIZE (1)
         0x95, 0x01,  //       REPORT_COUNT (1)
         0x81, 0x02,  //       INPUT (Data,Var,Abs)
         0x09, 0x32,  //       USAGE (In Range)
         0x81, 0x02,  //       INPUT (Data,Var,Abs)
         0x09, 0x47,  //       USAGE (Touch Valid)
         0x81, 0x02,  //       INPUT (Data,Var,Abs)
         0x95, 0x05,  //       REPORT_COUNT (5)
         0x81, 0x03,  //       INPUT (Cnst,Ary,Abs)
         0x25, MAX_TOUCH_POINT,  // LOGICAL_MAXIMUM (MAX_TOUCH_POINT)
         0x75, 0x08,  //       REPORT_SIZE (8)
         0x09, 0x51,  //       USAGE ( Contact Identifier)
         0x95, 0x01,  //       REPORT_COUNT (1)
         0x81, 0x02,  //       INPUT (Data,Var,Abs)
         0x05, 0x01,  //       USAGE_PAGE (Generic Desk..
         0x26, 0x00, 0x10,  //       LOGICAL_MAXIMUM (4096)
         0x75, 0x10,  //       REPORT_SIZE (16)
         0x55, 0x00,  //       UNIT_EXPONENT (0)
         0x65, 0x00,  //       UNIT (None)
         0x09, 0x30,  //       USAGE (X)
         0x35, 0x00,  //       PHYSICAL_MINIMUM (0)
         0x46, 0x00, 0x00,  //       PHYSICAL_MAXIMUM (0)
         0x81, 0x02,  //       INPUT (Data,Var,Abs)
         0x09, 0x31,  //       USAGE (Y)
         0x46, 0x00, 0x00,  //       PHYSICAL_MAXIMUM (0)
         0x81, 0x02,  //       INPUT (Data,Var,Abs)
         0xc0,        //    END_COLLECTION

         //这是一个全局条目，选择用途页为Digitizers
         0x05, 0x0d,  //    USAGE_PAGE (Digitizers)

         //这是一个局部条目，选择用途为Contact Count，即当前有多少点触摸
         0x09, 0x54,  //    USAGE (Contact Count)

         //这是一个全局条目，说明数据域的数量为1个。
         0x95, 0x01,  //    REPORT_COUNT (1)

         //这是一个全局条目，说明每个数据域的长度为8个bit。
         0x75, 0x08,  //    REPORT_SIZE (8)

         //这是一个全局条目，说明返回的数据的逻辑最小值为0。
         0x15, 0x00,  //    LOGICAL_MINIMUM (0)

         //这是一个全局条目，说明返回的数据的逻辑最大值为MAX_TOUCH_POINT。
         0x25, MAX_TOUCH_POINT,  //    LOGICAL_MAXIMUM (MAX_TOUCH_POINT)

         //这是一个主条目，说明有1个长度为8bit的数据域用于输入。
         //通过前面的描述我们可以知道，这个8bit是用来表示当前有几个点触摸，
         //0表示没有触摸，1表示触摸1个点触摸，2表示2个点触摸等等。
         0x81, 0x02,  //    INPUT (Data,Var,Abs)

         //这是一个局部条目，选择用途为Contact Count Maximum，
         //表示最多支持多少点同时触摸。
         0x09, 0x55,  //    USAGE(Contact Count Maximum)

         //这是一个主条目，说明有1个长度为8bit的数据域用于特性报告。
         //通过前面的描述我们可以知道，这个8bit是用来表示最多支持几个点触摸。
         0xb1, 0x02,  //    FEATURE (Data,Var,Abs)

         //这是一个主条目，关集合
         0xc0,        // END_COLLECTION
         */
        /***********************多点触摸描述完毕***************************/

        /*****************以下是配置设备的特性报告描述符*******************/
        /*
         //这是一个全局条目，选择用途页为Digitizers
         0x05, 0x0d,  //    USAGE_PAGE (Digitizers)

         //这是一个局部条目，选择用途为Device Configuration
         0x09, 0x0E,  // USAGE (Device Configuration)

         //这是一个主条目，开集合，后面跟的数据0x01表示该集合是一个应用集合。
         //它的性质在前面由用途页和用途定义为Digitizers的Device Configuration。
         0xa1, 0x01,  // COLLECTION (Application)

         //全局条目，定义特性报告ID为REPORTID_FEATURE(usbcore.h中定义)
         0x85, REPORTID_FEATURE,  //   REPORT_ID (Configuration)

         //这是一个局部条目，选择用途为Device Settings
         0x09, 0x23,  //   USAGE (Device Settings)

         //这是一个主条目，开集合，后面跟的数据0x02表示该集合是一个逻辑集合。
         0xa1, 0x02,  //   COLLECTION (logical)

         //这是一个局部条目，选择用途为Device Mode
         0x09, 0x52,  //    USAGE (Device Mode)

         //这是一个局部条目，选择用途为Device Identifier
         0x09, 0x53,  //    USAGE (Device Identifier)

         //这是一个全局条目，说明返回的数据的逻辑最小值为0。
         0x15, 0x00,  //    LOGICAL_MINIMUM (0)

         //这是一个全局条目，说明返回的数据的逻辑最大值为10。
         0x25, 0x0a,  //    LOGICAL_MAXIMUM (10)

         //这是一个全局条目，说明每个数据域的长度为8个bit。
         0x75, 0x08,  //    REPORT_SIZE (8)

         //这是一个全局条目，说明数据域的数量为2个。
         0x95, 0x02,  //    REPORT_COUNT (2)

         //这是一个主条目，说明有2个长度为8bit的数据域用于特性报告。
         //通过前面的描述我们可以知道，这2个8bit分别用来表示Device Mode和
         //Device Identifier。
         0xb1, 0x02,  //   FEATURE (Data,Var,Abs)
         0xc0,        //   END_COLLECTION
         0xc0        // END_COLLECTION
         */
        /***********************特性报告描述完毕***************************/
        };

//USB配置描述符集合的定义
//配置描述符总长度为9+9+9+7字节
uchar ConfigurationDescriptor[9 + 9 + 9 + 7] =
{
	/***************配置描述符***********************/
//bLength字段。配置描述符的长度为9字节。
        0x09,

        //bDescriptorType字段。配置描述符编号为0x02。
        0x02,

	//wTotalLength字段。配置描述符集合的总长度，
	//包括配置描述符本身、接口描述符、类描述符、端点描述符等。
	sizeof(ConfigurationDescriptor) & 0xFF,//低字节
	(sizeof(ConfigurationDescriptor) >> 8) & 0xFF,//高字节

        //bNumInterfaces字段。该配置包含的接口数，只有一个接口。
        0x01,

        //bConfiguration字段。该配置的值为1。
        0x01,

        //iConfigurationz字段，该配置的字符串索引。这里没有，为0。
        0x00,

        //bmAttributes字段，该设备的属性。由于我们的板子是总线供电的，
        //并且我们不想实现远程唤醒的功能，所以该字段的值为0x80。
        0x80,

        //bMaxPower字段，该设备需要的最大电流量。由于我们的板子
        //需要的电流不到100mA，因此我们这里设置为100mA。由于每单位
        //电流为2mA，所以这里设置为50(0x32)。
        0x32,

        /*******************接口描述符*********************/
        //bLength字段。接口描述符的长度为9字节。
        0x09,

        //bDescriptorType字段。接口描述符的编号为0x04。
        0x04,

        //bInterfaceNumber字段。该接口的编号，第一个接口，编号为0。
        0x00,

        //bAlternateSetting字段。该接口的备用编号，为0。
        0x00,

        //bNumEndpoints字段。非0端点的数目。由于USB触摸屏只需要一个
        //中断输入端点，因此该值为1。
        0x01,

        //bInterfaceClass字段。该接口所使用的类。USB触摸屏是HID类，
        //HID类的编码为0x03。
        0x03,

        //bInterfaceSubClass字段。该接口所使用的子类。在HID1.1协议中，
        //只规定了一种子类：支持BIOS引导启动的子类。
        //USB键盘、鼠标属于该子类，子类代码为0x01。
        0x00,

        //bInterfaceProtocol字段。如果子类为支持引导启动的子类，
        //则协议可选择鼠标和键盘。键盘代码为0x01，鼠标代码为0x02。
        0x00,

        //iConfiguration字段。该接口的字符串索引值。这里没有，为0。
        0x00,

        /******************HID描述符************************/
        //bLength字段。本HID描述符下只有一个下级描述符。所以长度为9字节。
        0x09,

        //bDescriptorType字段。HID描述符的编号为0x21。
        0x21,

        //bcdHID字段。本协议使用的HID1.1协议。注意低字节在先。
        0x10,
        0x01,

        //bCountyCode字段。设备适用的国家代码，这里选择为美国，代码0x21。
        0x21,

        //bNumDescriptors字段。下级描述符的数目。我们只有一个报告描述符。
        0x01,

        //bDescritporType字段。下级描述符的类型，为报告描述符，编号为0x22。
        0x22,

        //bDescriptorLength字段。下级描述符的长度。下级描述符为报告描述符。
        sizeof(ReportDescriptor) & 0xFF, (sizeof(ReportDescriptor) >> 8) & 0xFF,

        /**********************端点描述符***********************/
        //bLength字段。端点描述符长度为7字节。
        0x07,

        //bDescriptorType字段。端点描述符编号为0x05。
        0x05,

        //bEndpointAddress字段。端点的地址。我们使用D12的输入端点1。
        //D7位表示数据方向，输入端点D7为1。所以输入端点1的地址为0x81。
        0x81,

        //bmAttributes字段。D1~D0为端点传输类型选择。
        //该端点为中断端点。中断端点的编号为3。其它位保留为0。
        0x03,

        //wMaxPacketSize字段。该端点的最大包长。端点1的最大包长为16字节。
        //注意低字节在先。
        0x10,
        0x00,

        //bInterval字段。端点查询的时间，我们设置为10个帧时间，即10ms。
        0x01 };

#elif (USB_COMPLIE_MODE == USB_BULK)

uchar ConfigurationDescriptor[9 + 9 + 7 + 7] =
{
/***************配置描述符***********************/
//bLength字段。配置描述符的长度为9字节。
        0x09,

        //bDescriptorType字段。配置描述符编号为0x02。
        0x02,

        //wTotalLength字段。配置描述符集合的总长度，
        //包括配置描述符本身、接口描述符、类描述符、端点描述符等。
        sizeof(ConfigurationDescriptor) & 0xFF, //低字节
        (sizeof(ConfigurationDescriptor) >> 8) & 0xFF, //高字节

        //bNumInterfaces字段。该配置包含的接口数，只有一个接口。
        0x01,

        //bConfiguration字段。该配置的值为1。
        0x01,

        //iConfigurationz字段，该配置的字符串索引。这里没有，为0。
        0x05,

        //bmAttributes字段，该设备的属性。由于我们的板子是总线供电的，
        //并且我们不想实现远程唤醒的功能，所以该字段的值为0x80。
        0xc0,

        //bMaxPower字段，该设备需要的最大电流量。由于我们的板子
        //需要的电流不到100mA，因此我们这里设置为100mA。由于每单位
        //电流为2mA，所以这里设置为50(0x32)。
        0xfa,

        /*******************接口描述符*********************/
        //bLength字段。接口描述符的长度为9字节。
        0x09,

        //bDescriptorType字段。接口描述符的编号为0x04。
        0x04,

        //bInterfaceNumber字段。该接口的编号，第一个接口，编号为0。
        0x00,

        //bAlternateSetting字段。该接口的备用编号，为0。
        0x00,

        //bNumEndpoints字段。非0端点的数目。由于USB触摸屏只需要一个
        //中断输入端点，因此该值为1。
        0x02,

        //bInterfaceClass字段。该接口所使用的类。USB触摸屏是HID类，
        //HID类的编码为0x03。
        0xff,

        //bInterfaceSubClass字段。该接口所使用的子类。在HID1.1协议中，
        //只规定了一种子类：支持BIOS引导启动的子类。
        //USB键盘、鼠标属于该子类，子类代码为0x01。
        0x00,

        //bInterfaceProtocol字段。如果子类为支持引导启动的子类，
        //则协议可选择鼠标和键盘。键盘代码为0x01，鼠标代码为0x02。
        0x00,

        //iConfiguration字段。该接口的字符串索引值。这里没有，为0。
        0x04,

        /**********************端点描述符***********************/
        //bLength字段。端点描述符长度为7字节。
        0x07,

        //bDescriptorType字段。端点描述符编号为0x05。
        0x05,

        //bEndpointAddress字段。端点的地址。我们使用D12的输入端点1。
        //D7位表示数据方向，输入端点D7为1。所以输入端点1的地址为0x81。
        0x81,

        //bmAttributes字段。D1~D0为端点传输类型选择。
        //该端点为中断端点。中断端点的编号为3。其它位保留为0。
        0x02,

		//wMaxPacketSize字段。该端点的最大包长。端点1的最大包长为16字节。
		//注意低字节在先。
		0x40, 0x00,

        //bInterval字段。端点查询的时间，我们设置为10个帧时间，即10ms。
        0x01,
        //------------------out bulk-------------
         //bLength字段。端点描述符长度为7字节。
        0x07,

        //bDescriptorType字段。端点描述符编号为0x05。
        0x05,

        //bEndpointAddress字段。端点的地址。我们使用D12的输入端点1。
        //D7位表示数据方向，输入端点D7为1。所以输入端点1的地址为0x81。
        0x02,

        //bmAttributes字段。D1~D0为端点传输类型选择。
        //该端点为中断端点。中断端点的编号为3。其它位保留为0。
        0x02,

		//wMaxPacketSize字段。该端点的最大包长。端点1的最大包长为16字节。
		//注意低字节在先。
		0x40, 0x00,

		//bInterval字段。端点查询的时间，我们设置为10个帧时间，即10ms。
		0x01 };
#endif


extern sunxi_udc_t sunxi_udc_source;

static int __usb_get_descriptor(struct usb_device_request *req, uchar *buffer)
{
    int ret = SUNXI_USB_REQ_SUCCESSED;

    //获取描述符
    switch (req->wValue >> 8)
    {
    case USB_DT_DEVICE:		//设备描述符
    {
        sunxi_usb_fastboot_status = SUNXI_USB_FASTBOOT_SETUP;
        struct usb_device_descriptor *dev_dscrptr;

        sunxi_usb_dbg("get device descriptor\n");

        dev_dscrptr = (struct usb_device_descriptor *) buffer;
        memset((void *) dev_dscrptr, 0, sizeof(struct usb_device_descriptor));

		dev_dscrptr->bLength = MIN(req->wLength, sizeof(struct usb_device_descriptor));
		dev_dscrptr->bDescriptorType = USB_DT_DEVICE;
#ifdef CONFIG_USB_1_1_DEVICE
		dev_dscrptr->bcdUSB = 0x110;
#else
		dev_dscrptr->bcdUSB = 0x200;
#endif

#if (USB_COMPLIE_MODE == USB_HID)
		//---------HID-------------------------
		dev_dscrptr->bDeviceClass = 0x00;		//设备类：HID
		dev_dscrptr->bDeviceSubClass = 0x00;
		dev_dscrptr->bDeviceProtocol = 0x00;
		dev_dscrptr->bMaxPacketSize0 = 0x40;
		dev_dscrptr->idVendor = DEVICE_VENDOR_ID;
		dev_dscrptr->idProduct = DEVICE_PRODUCT_ID;
		dev_dscrptr->bcdDevice = DEVICE_BCD;
		dev_dscrptr->iManufacturer = 0x01;
		dev_dscrptr->iProduct = 0x02;
		dev_dscrptr->iSerialNumber = 00;
		dev_dscrptr->bNumConfigurations = 1;
		//-------end--HID-------------------------
#elif (USB_COMPLIE_MODE == USB_BULK)
		//--------start--bulk-------------------------- 
		dev_dscrptr->bDeviceClass = 0xff;		//设备类 bulk
		dev_dscrptr->bDeviceSubClass = 0x00;
		dev_dscrptr->bDeviceProtocol = 0x00;
		dev_dscrptr->bMaxPacketSize0 = 0x40;
		dev_dscrptr->idVendor = 0x0123;
		dev_dscrptr->idProduct = 0x0123;
		dev_dscrptr->bcdDevice = 0x100;
		dev_dscrptr->iManufacturer = 0x01;
		dev_dscrptr->iProduct = 0x02;
		dev_dscrptr->iSerialNumber = 0x03;
		dev_dscrptr->bNumConfigurations = 1;
		//--------end--bulk--------------------------
#endif
		sunxi_udc_send_setup(dev_dscrptr->bLength, buffer);

    }
        break;

    case USB_DT_CONFIG:		//配置描述符
    { 
		sunxi_usb_dbg("get config descriptor******* =%d\n", req->wLength);
		sunxi_udc_send_setup(MIN(req->wLength, sizeof(ConfigurationDescriptor)), ConfigurationDescriptor);
                #if (USB_COMPLIE_MODE == USB_BULK)        
                         
                #endif
	}
		break;

    case USB_DT_STRING:
    {
        unsigned char bLength = 0;
        unsigned char string_index = req->wValue & 0xff;

        sunxi_usb_dbg("get string descriptor\n");

        /* Language ID */
        if (string_index == 0)
        {
            bLength = MIN(4, req->wLength);

			sunxi_udc_send_setup(bLength, (void *) sunxi_usb_fastboot_dev[0]);
		}
		else if (string_index < SUNXI_USB_FASTBOOT_DEV_MAX)
		{
			/* Size of string in chars */
			unsigned char i = 0;
			unsigned char str_length = strlen((const char *) sunxi_usb_fastboot_dev[string_index]);
			unsigned char bLength = 2 + (2 * str_length);

            buffer[0] = bLength; /* length */
            buffer[1] = USB_DT_STRING; /* descriptor = string */

            /* Copy device string to fifo, expand to simple unicode */
            for (i = 0; i < str_length; i++)
            {
                buffer[2 + 2 * i + 0] = sunxi_usb_fastboot_dev[string_index][i];
                buffer[2 + 2 * i + 1] = 0;
            }
            bLength = MIN(bLength, req->wLength);

			sunxi_udc_send_setup(bLength, buffer);
		}
		else
		{
			//string_index = 5;
			unsigned char i = 0;
			unsigned char str_length = strlen((const char *) sunxi_usb_fastboot_dev[string_index]);
			unsigned char bLength = 2 + (2 * str_length);

            buffer[0] = bLength; /* length */
            buffer[1] = USB_DT_STRING; /* descriptor = string */

            /* Copy device string to fifo, expand to simple unicode */
            for (i = 0; i < str_length; i++)
            {
                buffer[2 + 2 * i + 0] = sunxi_usb_fastboot_dev[string_index][i];
                buffer[2 + 2 * i + 1] = 0;
            }
            bLength = MIN(bLength, req->wLength);

            sunxi_udc_send_setup(bLength, buffer);
            //printf("sunxi usb err: string line %d is not supported\n", string_index);
        }
    }
        break;

    case USB_DT_DEVICE_QUALIFIER:
    {
#ifdef CONFIG_USB_1_1_DEVICE
        /* This is an invalid request for usb 1.1, nak it */
        USBC_Dev_EpSendStall(sunxi_udc_source.usbc_hd, USBC_EP_TYPE_EP0);
#else
        struct usb_qualifier_descriptor *qua_dscrpt;

        sunxi_usb_dbg("get qualifier descriptor\n");

        qua_dscrpt = (struct usb_qualifier_descriptor *)buffer;
        memset(&buffer, 0, sizeof(struct usb_qualifier_descriptor));

        qua_dscrpt->bLength = MIN(req->wLength, sizeof(sizeof(struct usb_qualifier_descriptor)));
        qua_dscrpt->bDescriptorType = USB_DT_DEVICE_QUALIFIER;
        qua_dscrpt->bcdUSB = 0x200;
        qua_dscrpt->bDeviceClass = 0xff;
        qua_dscrpt->bDeviceSubClass = 0xff;
        qua_dscrpt->bDeviceProtocol = 0xff;
        qua_dscrpt->bMaxPacketSize0 = 0x40;
        qua_dscrpt->bNumConfigurations = 1;
        qua_dscrpt->bRESERVED = 0;

        sunxi_udc_send_setup(qua_dscrpt->bLength, buffer);
#endif
    }
        break;

	case REPORT_DESCRIPTOR:
	{
#if (USB_COMPLIE_MODE == USB_HID)
		sunxi_usb_dbg("REPORT_DESCRIPTOR =%d\n", req->wLength);
		sunxi_udc_send_setup(req->wLength, ReportDescriptor);
		//sunxi_udc_send_setup(MIN(req->wLength, sizeof(ReportDescriptor)), ReportDescriptor);
		sunxi_usb_fastboot_status = SUNXI_USB_FASTBOOT_RECEIVE_DATA;
		 
#endif
	}
		break;

    default:
        printf("err: unkown wValue(%d)\n", req->wValue);

        ret = SUNXI_USB_REQ_OP_ERR;
    }

    return ret;
}

/*
 *******************************************************************************
 *                     __usb_get_status
 *
 * Description:
 *    void
 *
 * Parameters:
 *    void
 *
 * Return value:
 *    void
 *
 * note:
 *    void
 *
 *******************************************************************************
 */
static int __usb_get_status(struct usb_device_request *req, uchar *buffer)
{
    unsigned char bLength = 0;

    sunxi_usb_dbg("get status\n");
    if (0 == req->wLength)
    {
        /* sent zero packet */
        sunxi_udc_send_setup(0, NULL);

        return SUNXI_USB_REQ_OP_ERR;
    }

    bLength = MIN(req->wValue, 2);

    buffer[0] = 1;
    buffer[1] = 0;

    sunxi_udc_send_setup(bLength, buffer);

    return SUNXI_USB_REQ_SUCCESSED;
}
/*
 ************************************************************************************************************
 *
 *                                             function
 *
 *    name          :
 *
 *    parmeters     :
 *
 *    return        :
 *
 *    note          :
 *
 *
 ************************************************************************************************************
 */
static int __sunxi_fastboot_send_status(void *buffer, unsigned int buffer_size)
{
    return sunxi_udc_send_data((uchar *) buffer, buffer_size);
}
/*
 *******************************************************************************
 *                     __fastboot_reboot
 *
 * Description:
 *    void
 *
 * Parameters:
 *    void
 *
 * Return value:
 *    void
 *
 * note:
 *    void
 *
 *******************************************************************************
 */
static int __fastboot_reboot(int word_mode)
{
    char response[8];

    sprintf(response, "OKAY");
    __sunxi_fastboot_send_status(response, strlen(response));
    __msdelay(1000); /* 1 sec */

    //sunxi_board_restart(word_mode);

    return 0;
}
/*
 *******************************************************************************
 *                     __erase_part
 *
 * Description:
 *    void
 *
 * Parameters:
 *    void
 *
 * Return value:
 *    void
 *
 * note:
 *    void
 *
 *******************************************************************************
 */
static int __erase_part(char *name)
{

    return 0;
}
/*
 *******************************************************************************
 *                     __flash_to_part
 *
 * Description:
 *    void
 *
 * Parameters:
 *    void
 *
 * Return value:
 *    void
 *
 * note:
 *    void
 *
 *******************************************************************************
 */
static void __flash_to_uboot(void)
{

    return;
}

static int __flash_to_part(char *name)
{

    return 0;
}

/*
 *******************************************************************************
 *                     __try_to_download
 *
 * Description:
 *    void
 *
 * Parameters:
 *    void
 *
 * Return value:
 *    void
 *
 * note:
 *    void
 *
 *******************************************************************************
 */
static int __try_to_download(char *download_size, char *response)
{
    int ret = -1;
    /*
     trans_data.try_to_recv = simple_strtoul (download_size, NULL, 16);
     all_download_bytes = trans_data.try_to_recv;
     printf("Starting download of %d BYTES\n", trans_data.try_to_recv);
     printf("Starting download of %d MB\n", trans_data.try_to_recv >> 20);

     if (0 == trans_data.try_to_recv)
     {
     //bad user input
     sprintf(response, "FAILdownload: data size is 0");
     }
     else if (trans_data.try_to_recv > SUNXI_USB_FASTBOOT_BUFFER_MAX)
     {
     sprintf(response, "FAILdownload: data > buffer");
     }
     else
     {
     // The default case, the transfer fits
     //   completely in the interface buffer
     sprintf(response, "DATA%08x", trans_data.try_to_recv);
     printf("download response: %s\n", response);

     ret = 0;
     }
     */
    return ret;
}
/*
 *******************************************************************************
 *                     __boot
 *
 * Description:
 *    void
 *
 * Parameters:
 *    void
 *
 * Return value:
 *    void
 *
 * note:
 *    void
 *
 *******************************************************************************
 */
static void __boot(void)
{
}
/*
 *******************************************************************************
 *                     __get_var
 *
 * Description:
 *    void
 *
 * Parameters:
 *    void
 *
 * Return value:
 *    void
 *
 * note:
 *    void
 *
 *******************************************************************************
 */
static void __get_var(char *ver_name)
{
    char response[68];

    memset(response, 0, 68);
    strcpy(response, "OKAY");

    if (!strcmp(ver_name, "version"))
    {
        strcpy(response + 4, "0.5");
    }
    else if (!strcmp(ver_name, "product"))
    {
        strcpy(response + 4, SUNXI_FASTBOOT_DEVICE_PRODUCT);
    }
    else if (!strcmp(ver_name, "serialno"))
    {
        strcpy(response + 4, SUNXI_FASTBOOT_DEVICE_SERIAL_NUMBER);
    }
    else if (!strcmp(ver_name, "downloadsize"))
    {
        sprintf(response + 4, "0x%08x", SUNXI_USB_FASTBOOT_BUFFER_MAX);
        printf("response: %s\n", response);
    }
    else if (!strcmp(ver_name, "secure"))
    {
        strcpy(response + 4, "yes");
    }
    else if (!strcmp(ver_name, "max-download-size"))
    {
        sprintf(response + 4, "0x%08x", SUNXI_USB_FASTBOOT_BUFFER_MAX);
        printf("response: %s\n", response);
    }
    else
    {
        strcpy(response + 4, "not supported");
    }

    __sunxi_fastboot_send_status(response, strlen(response));

    return;
}
/*
 ************************************************************************************************************
 *
 *                                             function
 *
 *    name          :
 *
 *    parmeters     :
 *
 *    return        :
 *
 *    note          :
 *
 *
 ************************************************************************************************************
 */
static void __oem_operation(char *operation)
{

    return;
}
/*
 ************************************************************************************************************
 *
 *                                             function
 *
 *    name          :
 *
 *    parmeters     :
 *
 *    return        :
 *
 *    note          :
 *
 *
 ************************************************************************************************************
 */
static void __continue(void)
{
    char response[32];

    memset(response, 0, 32);
    strcpy(response, "OKAY");

    __sunxi_fastboot_send_status(response, strlen(response));

    sunxi_usb_exit();
    /*
     if(uboot_spare_head.boot_data.storage_type)
     {
     setenv("bootcmd", "run setargs_mmc boot_normal");
     }
     else
     {
     setenv("bootcmd", "run setargs_nand boot_normal");
     }
     do_bootd(NULL, 0, 1, NULL);
     */
    return;
}
/*
 ************************************************************************************************************
 *
 *                                             function
 *
 *    name          :
 *
 *    parmeters     :
 *
 *    return        :
 *
 *    note          :
 *
 *
 ************************************************************************************************************
 */
static void __unsupported_cmd(void)
{
    char response[32];

    memset(response, 0, 32);
    strcpy(response, "FAIL");

    __sunxi_fastboot_send_status(response, strlen(response));

    return;
}
/*
 ************************************************************************************************************
 *
 *                                             function
 *
 *    name          :
 *
 *    parmeters     :
 *
 *    return        :
 *
 *    note          :
 *
 *
 ************************************************************************************************************
 */

#include "stdio.h"

int fputc(int ch, FILE *f)
{
    //USART2->DR = (u8) ch;
    standby_serial_putc((u8) ch);
    return ch;
}

char buf[SUNXI_FASTBOOT_SEND_MEM_SIZE];

static int sunxi_fastboot_init(void)
{
    printf("sunxi_fastboot_init\n");
    memset(&trans_data, 0, sizeof(fastboot_trans_set_t));
    sunxi_usb_fastboot_write_enable = 0;
    sunxi_usb_fastboot_status = SUNXI_USB_FASTBOOT_IDLE;

    all_download_bytes = 0;
    fastboot_data_flag = 0;

    trans_data.base_recv_buffer = (char *) FASTBOOT_TRANSFER_BUFFER;

    //trans_data.base_send_buffer = (char *)malloc(SUNXI_FASTBOOT_SEND_MEM_SIZE);
    trans_data.base_send_buffer = (char *) buf;
    if (!trans_data.base_send_buffer)
    {
        printf("sunxi usb fastboot err: unable to malloc memory for fastboot send\n");
        free(trans_data.base_recv_buffer);

        return -1;
    }
    printf("recv addr 0x%x\n", (uint) trans_data.base_recv_buffer);
    printf("send addr 0x%x\n", (uint) trans_data.base_send_buffer);

    return 0;
}
/*
 ************************************************************************************************************
 *
 *                                             function
 *
 *    name          :
 *
 *    parmeters     :
 *
 *    return        :
 *
 *    note          :
 *
 *
 ************************************************************************************************************
 */
static int sunxi_fastboot_exit(void)
{
    printf("sunxi_fastboot_exit\n");
    if (trans_data.base_send_buffer)
    {
        free(trans_data.base_send_buffer);
    }

    return 0;
}
/*
 ************************************************************************************************************
 *
 *                                             function
 *
 *    name          :
 *
 *    parmeters     :
 *
 *    return        :
 *
 *    note          :
 *
 *
 ************************************************************************************************************
 */
static void sunxi_fastboot_reset(void)
{
    sunxi_usb_fastboot_write_enable = 0;
    sunxi_usb_fastboot_status = SUNXI_USB_FASTBOOT_IDLE;
}
/*
 ************************************************************************************************************
 *
 *                                             function
 *
 *    name          :
 *
 *    parmeters     :
 *
 *    return        :
 *
 *    note          :
 *
 *
 ************************************************************************************************************
 */
static void sunxi_fastboot_usb_rx_dma_isr(void *p_arg)
{
    printf("dma int for usb rx occur\n");
    //通知主循环，可以写入数据
    sunxi_usb_fastboot_write_enable = 1;
}
/*
 ************************************************************************************************************
 *
 *                                             function
 *
 *    name          :
 *
 *    parmeters     :
 *
 *    return        :
 *
 *    note          :
 *
 *
 ************************************************************************************************************
 */
static void sunxi_fastboot_usb_tx_dma_isr(void *p_arg)
{
    printf("dma int for usb tx occur\n");
}
/*
 ************************************************************************************************************
 *
 *                                             function
 *
 *    name          :
 *
 *    parmeters     :
 *
 *    return        :
 *
 *    note          :
 *
 *
 ************************************************************************************************************
 */
static int sunxi_fastboot_standard_req_op(uint cmd, struct usb_device_request *req, uchar *buffer)
{
    int ret = SUNXI_USB_REQ_OP_ERR;

    switch (cmd)
    {
    case USB_REQ_GET_STATUS:
    {
        ret = __usb_get_status(req, buffer);

        break;
    }
        //case USB_REQ_CLEAR_FEATURE:
        //case USB_REQ_SET_FEATURE:
    case USB_REQ_SET_ADDRESS:
    {
        ret = __usb_set_address(req);

        break;
    }
    case USB_REQ_GET_DESCRIPTOR:
    {
        ret = __usb_get_descriptor(req, buffer); //debug wg
        break;
    }

        //case USB_REQ_SET_DESCRIPTOR:
    case USB_REQ_GET_CONFIGURATION:
    {
        ret = __usb_get_descriptor(req, buffer);

        break;
    }
    case USB_REQ_SET_CONFIGURATION:
    {
        ret = __usb_set_configuration(req);

        break;
    }
        //case USB_REQ_GET_INTERFACE:
    case USB_REQ_SET_INTERFACE:
    {
        ret = __usb_set_interface(req);

        break;
    }
        //case USB_REQ_SYNCH_FRAME:
    default:
    {
        //tick_printf("sunxi fastboot error: standard req is not supported\n");

        ret = SUNXI_USB_REQ_DEVICE_NOT_SUPPORTED;

        break;
    }
    }

    return ret;
}
/*
 ************************************************************************************************************
 *
 *                                             function
 *
 *    name          :
 *
 *    parmeters     :
 *
 *    return        :
 *
 *    note          :
 *
 *
 ************************************************************************************************************
 */
static int sunxi_fastboot_nonstandard_req_op(uint cmd, struct usb_device_request *req, uchar *buffer, uint data_status)
{
    return SUNXI_USB_REQ_DEVICE_NOT_SUPPORTED;
}

/*
 ************************************************************************************************************
 *
 *                                             function
 *
 *    name          :
 *
 *    parmeters     :
 *
 *    return        :
 *
 *    note          :
 *
 *
 ************************************************************************************************************
 */
static void __limited_fastboot(void)
{
    char response[64];

    memset(response, 0, 64);
    //tick_printf("secure mode,fastboot limited used\n");

    //strcpy(response,"FAIL:secure mode,fastboot limited used");

    //__sunxi_fastboot_send_status(response, strlen(response));

    return;
}

/*
 ************************************************************************************************************
 *
 *                                             function
 *
 *    name          :
 *
 *    parmeters     :
 *
 *    return        :
 *
 *    note          :
 *
 *
 ************************************************************************************************************
 */
static int sunxi_fastboot_locked(void)
{
    //if ((gd->securemode == SUNXI_SECURE_MODE_WITH_SECUREOS) || (gd->securemode == SUNXI_SECURE_MODE_NO_SECUREOS))
    {
        printf("the system is secure\n");
        return 0;
    }
    //return -1;
}
/*
 ************************************************************************************************************
 *
 *                                             function
 *
 *    name          :
 *
 *    parmeters     :
 *
 *    return        :
 *
 *    note          :
 *
 *
 ************************************************************************************************************
 */

int Sentmakr = 0;
static int sunxi_fastboot_state_loop(void *buffer)
{
    int ret;
    sunxi_ubuf_t *sunxi_ubuf = (sunxi_ubuf_t *) buffer;
    char response[68];

    switch (sunxi_usb_fastboot_status)
    {
    case SUNXI_USB_FASTBOOT_IDLE:

        break;
    case SUNXI_USB_FASTBOOT_RECEIVE_DATA:
        if (Sentmakr)
        { 
            sunxi_usb_fastboot_status = SUNXI_USB_FASTBOOT_SEND_DATA;
            Sentmakr = 0;
        }
        break;

    default:
        break;
    }
    /*
     switch(sunxi_usb_fastboot_status)
     {
     case SUNXI_USB_FASTBOOT_IDLE:
     if(sunxi_ubuf->rx_ready_for_data == 1)
     {
     sunxi_usb_fastboot_status = SUNXI_USB_FASTBOOT_SETUP;
     }

     if(Sentmakr )
     {
     __sunxi_fastboot_send_status("1234456",4);
     Sentmakr = 0;
     }

     break;

     case SUNXI_USB_FASTBOOT_SETUP:
     printf("SUNXI_USB_FASTBOOT_SETUP\n");
     //tick_printf("SUNXI_USB_FASTBOOT_SETUP\n");

     //tick_printf("fastboot command = %s\n", sunxi_ubuf->rx_req_buffer);

     sunxi_usb_fastboot_status = SUNXI_USB_FASTBOOT_IDLE;
     sunxi_ubuf->rx_ready_for_data = 0;
     if(memcmp(sunxi_ubuf->rx_req_buffer, "reboot-bootloader", strlen("reboot-bootloader")) == 0)
     {
     tick_printf("reboot-bootloader\n");
     //__fastboot_reboot(PMU_PRE_FASTBOOT_MODE);
     }
     else if(memcmp(sunxi_ubuf->rx_req_buffer, "reboot", 6) == 0)
     {
     tick_printf("reboot\n");
     __fastboot_reboot(0);
     }
     else if(memcmp(sunxi_ubuf->rx_req_buffer, "erase:", 6) == 0)
     {
     tick_printf("erase\n");
     if(!sunxi_fastboot_locked())
     {
     __limited_fastboot();
     break;
     }
     __erase_part((char *)(sunxi_ubuf->rx_req_buffer + 6));
     }
     else if(memcmp(sunxi_ubuf->rx_req_buffer, "flash:", 6) == 0)
     {
     tick_printf("flash\n");
     if(!sunxi_fastboot_locked())
     {
     __limited_fastboot();
     break;
     }
     if(memcmp((char *)(sunxi_ubuf->rx_req_buffer + 6),"u-boot",6) == 0)
     __flash_to_uboot();
     else
     __flash_to_part((char *)(sunxi_ubuf->rx_req_buffer + 6));
     }
     else if(memcmp(sunxi_ubuf->rx_req_buffer, "download:", 9) == 0)
     {
     tick_printf("download\n");
     if(!sunxi_fastboot_locked())
     {
     __limited_fastboot();
     break;
     }
     ret = __try_to_download((char *)(sunxi_ubuf->rx_req_buffer + 9), response);
     if(ret >= 0)
     {
     fastboot_data_flag = 1;
     sunxi_ubuf->rx_req_buffer  = (uchar *)trans_data.base_recv_buffer;
     sunxi_usb_fastboot_status  = SUNXI_USB_FASTBOOT_RECEIVE_DATA;
     }
     __sunxi_fastboot_send_status(response, strlen(response));
     }
     else if(memcmp(sunxi_ubuf->rx_req_buffer, "boot", 4) == 0)
     {
     tick_printf("boot\n");
     if(!sunxi_fastboot_locked())
     {
     __limited_fastboot();
     break;
     }
     __boot();
     }
     else if(memcmp(sunxi_ubuf->rx_req_buffer, "getvar:", 7) == 0)
     {
     tick_printf("getvar\n");
     if(!sunxi_fastboot_locked())
     {
     __limited_fastboot();
     break;
     }
     __get_var((char *)(sunxi_ubuf->rx_req_buffer + 7));
     }
     else if(memcmp(sunxi_ubuf->rx_req_buffer, "oem", 3) == 0)
     {
     tick_printf("oem operations\n");
     if(!sunxi_fastboot_locked())
     {
     __limited_fastboot();
     break;
     }
     __oem_operation((char *)(sunxi_ubuf->rx_req_buffer + 4));
     }
     else if(memcmp(sunxi_ubuf->rx_req_buffer, "continue", 8) == 0)
     {
     tick_printf("continue\n");
     __continue();
     }
     else
     {
     tick_printf("not supported fastboot cmd\n");
     __unsupported_cmd();
     }

     break;

     case SUNXI_USB_FASTBOOT_SEND_DATA:

     printf("SUNXI_USB_FASTBOOT_SEND_DATA\n");

     break;

     case SUNXI_USB_FASTBOOT_RECEIVE_DATA:

     //tick_printf("SUNXI_USB_FASTBOOT_RECEIVE_DATA\n");
     if((fastboot_data_flag == 1) && ((char *)sunxi_ubuf->rx_req_buffer == all_download_bytes + trans_data.base_recv_buffer))	//传输完毕
     {
     tick_printf("fastboot transfer finish\n");
     fastboot_data_flag = 0;
     sunxi_usb_fastboot_status  = SUNXI_USB_FASTBOOT_IDLE;

     sunxi_ubuf->rx_req_buffer = sunxi_ubuf->rx_base_buffer;

     sprintf(response,"OKAY");
     __sunxi_fastboot_send_status(response, strlen(response));
     }

     break;

     default:
     break;
     } 
     */

    return 0;
}
  

sunxi_usb_module_init(SUNXI_USB_DEVICE_FASTBOOT, sunxi_fastboot_init,
                      sunxi_fastboot_exit, sunxi_fastboot_reset,
                      sunxi_fastboot_standard_req_op,
                      sunxi_fastboot_nonstandard_req_op,
                      sunxi_fastboot_state_loop, sunxi_fastboot_usb_rx_dma_isr,
                      sunxi_fastboot_usb_tx_dma_isr);

extern sunxi_usb_setup_req_t *sunxi_udev_active;

void FastInit(void)
{
    sunxi_usb_module_reg(SUNXI_USB_DEVICE_FASTBOOT);
    sunxi_usb_main_loop(1);
}
