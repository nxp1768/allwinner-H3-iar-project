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
//--------���㴥���豸�豸����
uchar ReportDescriptor[] =
{
/***********************�����ǵ��㴥������������********************/
//ÿ�п�ʼ�ĵ�һ�ֽ�Ϊ����Ŀ��ǰ׺��ǰ׺�ĸ�ʽΪ��
//D7~D4��bTag��D3~D2��bType��D1~D0��bSize�����·ֱ��ÿ����Ŀע�͡�
//����һ��ȫ�֣�bTypeΪ1����Ŀ��ѡ����;ҳΪ��ͨ����Generic Desktop Page(0x01)
//�����һ�ֽ����ݣ�bSizeΪ1����������ֽ����Ͳ�ע���ˣ�
//�Լ�����bSize���жϡ�
        0x05,
        0x01, // USAGE_PAGE (Generic Desktop)

        //����һ���ֲ���bTypeΪ2����Ŀ��˵����������Ӧ�ü�����;�������
        0x09,
        0x02, // USAGE (Mouse)

        //����һ������bTypeΪ0����Ŀ�������ϣ������������0x01��ʾ
        //�ü�����һ��Ӧ�ü��ϡ�����������ǰ������;ҳ����;����Ϊ
        //��ͨ�����õ���ꡣ
        0xa1,
        0x01, // COLLECTION (Application)

        //ȫ����Ŀ�����嵥�㴥���ı���IDΪREPORTID_STOUCH(usbcore.h�ж���)
        0x85,
        REPORTID_STOUCH, //   REPORT_ID (Single Touch)

        //����һ���ֲ���Ŀ��˵����;Ϊָ�뼯��
        0x09,
        0x01, //   USAGE (Pointer)

        //����һ������Ŀ�������ϣ������������0x00��ʾ�ü�����һ��
        //�����ϣ���;��ǰ��ľֲ���Ŀ����Ϊָ�뼯�ϡ�
        0xa1,
        0x00, //   COLLECTION (Physical)

        //����һ��ȫ����Ŀ��ѡ����;ҳΪ������Button Page(0x09)��
        0x05,
        0x09, //     USAGE_PAGE (Button)

        //����һ���ֲ���Ŀ��˵����;����СֵΪ1��ʵ��������������
        0x19,
        0x01, //     USAGE_MINIMUM (Button 1)

        //����һ���ֲ���Ŀ��˵����;�����ֵΪ3��ʵ����������м���
        0x29,
        0x03, //     USAGE_MAXIMUM (Button 3)

        //����һ��ȫ����Ŀ��˵�����ص����ݵ��߼�ֵ���������Ƿ��ص��������ֵ����
        //��СΪ0����Ϊ����������Bit����ʾһ�������������СΪ0�����Ϊ1��
        0x15,
        0x00, //     LOGICAL_MINIMUM (0)

        //����һ��ȫ����Ŀ��˵���߼�ֵ���Ϊ1��
        0x25,
        0x01, //     LOGICAL_MAXIMUM (1)

        //����һ��ȫ����Ŀ��˵�������������Ϊ������
        0x95,
        0x03, //     REPORT_COUNT (3)

        //����һ��ȫ����Ŀ��˵��ÿ��������ĳ���Ϊ1��bit��
        0x75,
        0x01, //     REPORT_SIZE (1)

        //����һ������Ŀ��˵����3������Ϊ1bit�������������ͳ���
        //��ǰ�������ȫ����Ŀ�����壩������Ϊ���룬
        //����Ϊ��Data,Var,Abs��Data��ʾ��Щ���ݿ��Ա䶯��Var��ʾ
        //��Щ�������Ƕ����ģ�ÿ�����ʾһ����˼��Abs��ʾ����ֵ��
        //��������Ľ�����ǣ���һ��������bit0��ʾ����1��������Ƿ��£�
        //�ڶ���������bit1��ʾ����2���Ҽ����Ƿ��£�������������bit2��ʾ
        //����3���м����Ƿ��¡�
        0x81,
        0x02, //     INPUT (Data,Var,Abs)

        //����һ��ȫ����Ŀ��˵������������Ϊ1��
        0x95,
        0x01, //     REPORT_COUNT (1)

        //����һ��ȫ����Ŀ��˵��ÿ��������ĳ���Ϊ5bit��
        0x75,
        0x05, //     REPORT_SIZE (5)

        //����һ������Ŀ�������ã���ǰ������ȫ����Ŀ��֪������Ϊ5bit��
        //����Ϊ1������������Ϊ�����������ص�����һֱ��0����
        //���ֻ��Ϊ�˴���һ���ֽڣ�ǰ������3��bit��������һЩ����
        //���ѣ���������û��ʵ����;�ġ�
        0x81,
        0x03, //     INPUT (Cnst,Var,Abs)

        //����һ��ȫ����Ŀ��ѡ����;ҳΪ��ͨ����Generic Desktop Page(0x01)
        0x05,
        0x01, //     USAGE_PAGE (Generic Desktop)

        //����һ���ֲ���Ŀ��˵����;ΪX��
        0x09,
        0x30, //     USAGE (X)

        //����һ���ֲ���Ŀ��˵����;ΪY��
        0x09,
        0x31, //     USAGE (Y)

        //��������Ϊȫ����Ŀ��˵�����ص��߼���С�����ֵ��
        //���ﶨ��X��Y����߼���СֵΪ0��������ԭ��
        //X��Y����߼����ֵΪ32768������Ļ���·�������Ϊ��32768��32768����
        //����32768������һ�ֽڵķ�Χ��������Ҫ��2�ֽڵĸ�ʽ��ʾ���ֵ
        0x15,
        0x00, //     LOGICAL_MINIMUM (0)
        0x26, 0xFF, 0x7F, //     LOGICAL_MAXIMUM (32768)

        //��������Ϊȫ����Ŀ��˵�����ص�������С�����ֵ��
        //���ﶨ��X��Y���������СֵΪ0��������ԭ��
        //X��Y����������ֵΪ32768������Ļ���·�������Ϊ��32768��32768����
        //����4096������һ�ֽڵķ�Χ��������Ҫ��2�ֽڵĸ�ʽ��ʾ���ֵ
        0x35,
        0x00,    //Physical Minimum (0)
        0x46, 0xFF, 0x7F, //Physical Maximum(32768)

        //����һ��ȫ����Ŀ��˵��������ĳ���Ϊ16bit��
        0x75, 0x10, //     REPORT_SIZE (16)

        //����һ��ȫ����Ŀ��˵��������ĸ���Ϊ2����
        0x95,
        0x02, //     REPORT_COUNT (2)

        //����һ������Ŀ����˵��������16bit���������������õģ�
        //����Ϊ��Data,Var,Abs��Data˵�������ǿ��Ա�ģ�Var˵��
        //��Щ�������Ƕ����ģ�Abs��ʾ��Щֵ�Ǿ���ֵ��
        0x81,
        0x02, //     INPUT (Data,Var,Abs)

        //��������������Ŀ�����ر�ǰ��ļ����á�
        //���ǿ����������ϣ�����Ҫ�����Ρ�bSizeΪ0�����Ժ���û���ݡ�
        0xc0,//   END_COLLECTION
        0xc0,       // END_COLLECTION

        /***********************���㴥���������***************************/

        /***********************�����Ƕ�㴥����������*********************/
        /*
         //����һ��ȫ����Ŀ��ѡ����;ҳΪDigitizers
         0x05, 0x0d,  // USAGE_PAGE (Digitizers)

         //����һ���ֲ���Ŀ��˵����������Ӧ�ü�����;����Touch Screen
         0x09, 0x04,  // USAGE (Touch Screen)

         //����һ������Ŀ�������ϣ������������0x01��ʾ�ü�����һ��Ӧ�ü��ϡ�
         //����������ǰ������;ҳ����;����ΪDigitizers��Touch Screen��
         0xa1, 0x01,  // COLLECTION (Application)

         //ȫ����Ŀ�������㴥���ı���IDΪREPORTID_MTOUCH(usbcore.h�ж���)
         0x85, REPORTID_MTOUCH,  //   REPORT_ID (Touch)

         //����һ���ֲ���Ŀ��ѡ����;ΪDigitizersҳ�е�Finger
         0x09, 0x22,  //   USAGE (Finger)

         //����һ������Ŀ�������ϣ������������0x02��ʾ�ü�����һ���߼����ϡ�
         //����������ǰ������;ҳ����;����ΪDigitizers��Finger��
         0xa1, 0x02,  //     COLLECTION (Logical)

         //����һ���ֲ���Ŀ��ѡ����;ΪDigitizersҳ�е�Tip Switch��
         //����������ʾ�������Ƿ�Ӵ�����Ļ������������ʾ�Ƿ�����ָ������
         0x09, 0x42,  //       USAGE (Tip Switch)

         //����һ��ȫ����Ŀ��˵�����ص����ݵ��߼���СֵΪ0��
         0x15, 0x00,  //       LOGICAL_MINIMUM (0)

         //����һ��ȫ����Ŀ��˵�����ص����ݵ��߼����ֵΪ1��
         0x25, 0x01,  //       LOGICAL_MAXIMUM (1)

         //����һ��ȫ����Ŀ��˵��ÿ��������ĳ���Ϊ1��bit��
         0x75, 0x01,  //       REPORT_SIZE (1)

         //����һ��ȫ����Ŀ��˵�������������Ϊ1����
         0x95, 0x01,  //       REPORT_COUNT (1)

         //����һ������Ŀ��˵����1������Ϊ1bit���������������롣
         //ͨ��ǰ����������ǿ���֪�������bit��������ʾ�Ƿ����ģ�
         //0��ʾû�д�����1��ʾ������
         0x81, 0x02,  //       INPUT (Data,Var,Abs)

         //����һ���ֲ���Ŀ��ѡ����;ΪDigitizersҳ�е�In Range��
         //������ʾ�����������Ƿ���Ч��
         0x09, 0x32,  //       USAGE (In Range)

         //����һ������Ŀ��˵����1������Ϊ1bit���������������롣
         //ͨ��ǰ����������ǿ���֪�������bit��������ʾ���������Ƿ���Ч�ģ�
         //0��ʾ��Ч��1��ʾ��Ч��
         0x81, 0x02,  //       INPUT (Data,Var,Abs)

         //����һ���ֲ���Ŀ��ѡ����;ΪDigitizersҳ�е�Touch Valid��
         //������ʾ�����Ƿ���Ч��
         0x09, 0x47,  //       USAGE (Touch Valid)

         //����һ������Ŀ��˵����1������Ϊ1bit���������������롣
         //ͨ��ǰ����������ǿ���֪�������bit��������ʾ�����Ƿ���Ч�ģ�
         //0��ʾ��Ч��1��ʾ��Ч��
         0x81, 0x02,  //       INPUT (Data,Var,Abs)

         //����һ��ȫ����Ŀ��˵�������������Ϊ5����
         0x95, 0x05,  //       REPORT_COUNT (5)

         //����һ������Ŀ�������ã���ǰ������ȫ����Ŀ��֪������Ϊ1bit��
         //����Ϊ5������������Ϊ�����������ص�����һֱ��0����
         //���ֻ��Ϊ�˴���һ���ֽڣ�ǰ������3��bit��������һЩ����
         //���ѣ���������û��ʵ����;�ġ�
         0x81, 0x03,  //       INPUT (Cnst,Ary,Abs)

         //����һ��ȫ����Ŀ��˵�����ص����ݵ��߼����ֵΪMAX_TOUCH_POINT��
         0x25, MAX_TOUCH_POINT,  // LOGICAL_MAXIMUM (MAX_TOUCH_POINT)

         //����һ��ȫ����Ŀ��˵��ÿ��������ĳ���Ϊ8��bit��
         0x75, 0x08,  //       REPORT_SIZE (8)

         //����һ���ֲ���Ŀ��ѡ����;ΪDigitizersҳ�е�Contact Identifier��
         //������ʾ������ID�š�
         0x09, 0x51,  //       USAGE (Contact Identifier)

         //����һ��ȫ����Ŀ��˵�������������Ϊ1����
         0x95, 0x01,  //       REPORT_COUNT (1)

         //����һ������Ŀ��˵����1������Ϊ8bit���������������롣
         //ͨ��ǰ����������ǿ���֪�������8bit��������ʾ����ID�ŵģ�
         0x81, 0x02,  //       INPUT (Data,Var,Abs)

         //����һ��ȫ����Ŀ��ѡ����;ҳΪGeneric Desktop
         0x05, 0x01,  //       USAGE_PAGE (Generic Desktop)

         //����һ��ȫ����Ŀ��˵�����ص����ݵ��߼����ֵΪ4096��
         0x26, 0x00, 0x10,  //       LOGICAL_MAXIMUM (4096)

         //����һ��ȫ����Ŀ��˵��ÿ��������ĳ���Ϊ16��bit��
         0x75, 0x10,  //       REPORT_SIZE (16)

         //����һ��ȫ����Ŀ��˵������λ��ָ��Ϊ0
         0x55, 0x00,  //       UNIT_EXPONENT (0)

         //����һ��ȫ����Ŀ��˵��û������λ
         0x65, 0x00,  //       UNIT (None)

         //����һ���ֲ���Ŀ��ѡ����;Ϊͨ�������е�X��
         0x09, 0x30,  //       USAGE (X)

         //����һ��ȫ����Ŀ��˵��������СֵΪ0�����������ò���������С��
         //���ֵ�����Խ�������С�����ֵ������0��������λ�������ޡ�
         0x35, 0x00,  //       PHYSICAL_MINIMUM (0)

         //����һ��ȫ����Ŀ��˵���������ֵΪ0��
         0x46, 0x00, 0x00,  //       PHYSICAL_MAXIMUM (0)

         //����һ������Ŀ����˵�����16bit���������������õģ�
         //����Ϊ��Data,Var,Abs��Data˵�������ǿ��Ա�ģ�Var˵��
         //��Щ�������Ƕ����ģ�Abs��ʾ��Щֵ�Ǿ���ֵ������ʵ����
         //����Ҫ���ص�X�������ݡ�
         0x81, 0x02,  //       INPUT (Data,Var,Abs)

         //����һ���ֲ���Ŀ��ѡ����;Ϊͨ�������е�Y��
         0x09, 0x31,  //       USAGE (Y)

         //����һ��ȫ����Ŀ��˵���������ֵΪ0��
         0x46, 0x00, 0x00,  //       PHYSICAL_MAXIMUM (0)

         //����һ������Ŀ����˵�����16bit���������������õģ�
         //����Ϊ��Data,Var,Abs��Data˵�������ǿ��Ա�ģ�Var˵��
         //��Щ�������Ƕ����ģ�Abs��ʾ��Щֵ�Ǿ���ֵ������ʵ����
         //����Ҫ���ص�Y�������ݡ�
         0x81, 0x02,  //       INPUT (Data,Var,Abs)

         //����һ������Ŀ���ر�ǰ�濪���߼�����
         0xc0,        //    END_COLLECTION

         //������һֱ�������END_COLLECTION������������һ���߼����ϵ��ظ���
         //����Ͳ��ٸ���ע���ˣ���������з�����
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

         //����һ��ȫ����Ŀ��ѡ����;ҳΪDigitizers
         0x05, 0x0d,  //    USAGE_PAGE (Digitizers)

         //����һ���ֲ���Ŀ��ѡ����;ΪContact Count������ǰ�ж��ٵ㴥��
         0x09, 0x54,  //    USAGE (Contact Count)

         //����һ��ȫ����Ŀ��˵�������������Ϊ1����
         0x95, 0x01,  //    REPORT_COUNT (1)

         //����һ��ȫ����Ŀ��˵��ÿ��������ĳ���Ϊ8��bit��
         0x75, 0x08,  //    REPORT_SIZE (8)

         //����һ��ȫ����Ŀ��˵�����ص����ݵ��߼���СֵΪ0��
         0x15, 0x00,  //    LOGICAL_MINIMUM (0)

         //����һ��ȫ����Ŀ��˵�����ص����ݵ��߼����ֵΪMAX_TOUCH_POINT��
         0x25, MAX_TOUCH_POINT,  //    LOGICAL_MAXIMUM (MAX_TOUCH_POINT)

         //����һ������Ŀ��˵����1������Ϊ8bit���������������롣
         //ͨ��ǰ����������ǿ���֪�������8bit��������ʾ��ǰ�м����㴥����
         //0��ʾû�д�����1��ʾ����1���㴥����2��ʾ2���㴥���ȵȡ�
         0x81, 0x02,  //    INPUT (Data,Var,Abs)

         //����һ���ֲ���Ŀ��ѡ����;ΪContact Count Maximum��
         //��ʾ���֧�ֶ��ٵ�ͬʱ������
         0x09, 0x55,  //    USAGE(Contact Count Maximum)

         //����һ������Ŀ��˵����1������Ϊ8bit���������������Ա��档
         //ͨ��ǰ����������ǿ���֪�������8bit��������ʾ���֧�ּ����㴥����
         0xb1, 0x02,  //    FEATURE (Data,Var,Abs)

         //����һ������Ŀ���ؼ���
         0xc0,        // END_COLLECTION
         */
        /***********************��㴥���������***************************/

        /*****************�����������豸�����Ա���������*******************/
        /*
         //����һ��ȫ����Ŀ��ѡ����;ҳΪDigitizers
         0x05, 0x0d,  //    USAGE_PAGE (Digitizers)

         //����һ���ֲ���Ŀ��ѡ����;ΪDevice Configuration
         0x09, 0x0E,  // USAGE (Device Configuration)

         //����һ������Ŀ�������ϣ������������0x01��ʾ�ü�����һ��Ӧ�ü��ϡ�
         //����������ǰ������;ҳ����;����ΪDigitizers��Device Configuration��
         0xa1, 0x01,  // COLLECTION (Application)

         //ȫ����Ŀ���������Ա���IDΪREPORTID_FEATURE(usbcore.h�ж���)
         0x85, REPORTID_FEATURE,  //   REPORT_ID (Configuration)

         //����һ���ֲ���Ŀ��ѡ����;ΪDevice Settings
         0x09, 0x23,  //   USAGE (Device Settings)

         //����һ������Ŀ�������ϣ������������0x02��ʾ�ü�����һ���߼����ϡ�
         0xa1, 0x02,  //   COLLECTION (logical)

         //����һ���ֲ���Ŀ��ѡ����;ΪDevice Mode
         0x09, 0x52,  //    USAGE (Device Mode)

         //����һ���ֲ���Ŀ��ѡ����;ΪDevice Identifier
         0x09, 0x53,  //    USAGE (Device Identifier)

         //����һ��ȫ����Ŀ��˵�����ص����ݵ��߼���СֵΪ0��
         0x15, 0x00,  //    LOGICAL_MINIMUM (0)

         //����һ��ȫ����Ŀ��˵�����ص����ݵ��߼����ֵΪ10��
         0x25, 0x0a,  //    LOGICAL_MAXIMUM (10)

         //����һ��ȫ����Ŀ��˵��ÿ��������ĳ���Ϊ8��bit��
         0x75, 0x08,  //    REPORT_SIZE (8)

         //����һ��ȫ����Ŀ��˵�������������Ϊ2����
         0x95, 0x02,  //    REPORT_COUNT (2)

         //����һ������Ŀ��˵����2������Ϊ8bit���������������Ա��档
         //ͨ��ǰ����������ǿ���֪������2��8bit�ֱ�������ʾDevice Mode��
         //Device Identifier��
         0xb1, 0x02,  //   FEATURE (Data,Var,Abs)
         0xc0,        //   END_COLLECTION
         0xc0        // END_COLLECTION
         */
        /***********************���Ա����������***************************/
        };

//USB�������������ϵĶ���
//�����������ܳ���Ϊ9+9+9+7�ֽ�
uchar ConfigurationDescriptor[9 + 9 + 9 + 7] =
{
	/***************����������***********************/
//bLength�ֶΡ������������ĳ���Ϊ9�ֽڡ�
        0x09,

        //bDescriptorType�ֶΡ��������������Ϊ0x02��
        0x02,

	//wTotalLength�ֶΡ��������������ϵ��ܳ��ȣ�
	//�������������������ӿ��������������������˵��������ȡ�
	sizeof(ConfigurationDescriptor) & 0xFF,//���ֽ�
	(sizeof(ConfigurationDescriptor) >> 8) & 0xFF,//���ֽ�

        //bNumInterfaces�ֶΡ������ð����Ľӿ�����ֻ��һ���ӿڡ�
        0x01,

        //bConfiguration�ֶΡ������õ�ֵΪ1��
        0x01,

        //iConfigurationz�ֶΣ������õ��ַ�������������û�У�Ϊ0��
        0x00,

        //bmAttributes�ֶΣ����豸�����ԡ��������ǵİ��������߹���ģ�
        //�������ǲ���ʵ��Զ�̻��ѵĹ��ܣ����Ը��ֶε�ֵΪ0x80��
        0x80,

        //bMaxPower�ֶΣ����豸��Ҫ�������������������ǵİ���
        //��Ҫ�ĵ�������100mA�����������������Ϊ100mA������ÿ��λ
        //����Ϊ2mA��������������Ϊ50(0x32)��
        0x32,

        /*******************�ӿ�������*********************/
        //bLength�ֶΡ��ӿ��������ĳ���Ϊ9�ֽڡ�
        0x09,

        //bDescriptorType�ֶΡ��ӿ��������ı��Ϊ0x04��
        0x04,

        //bInterfaceNumber�ֶΡ��ýӿڵı�ţ���һ���ӿڣ����Ϊ0��
        0x00,

        //bAlternateSetting�ֶΡ��ýӿڵı��ñ�ţ�Ϊ0��
        0x00,

        //bNumEndpoints�ֶΡ���0�˵����Ŀ������USB������ֻ��Ҫһ��
        //�ж�����˵㣬��˸�ֵΪ1��
        0x01,

        //bInterfaceClass�ֶΡ��ýӿ���ʹ�õ��ࡣUSB��������HID�࣬
        //HID��ı���Ϊ0x03��
        0x03,

        //bInterfaceSubClass�ֶΡ��ýӿ���ʹ�õ����ࡣ��HID1.1Э���У�
        //ֻ�涨��һ�����ࣺ֧��BIOS�������������ࡣ
        //USB���̡�������ڸ����࣬�������Ϊ0x01��
        0x00,

        //bInterfaceProtocol�ֶΡ��������Ϊ֧���������������࣬
        //��Э���ѡ�����ͼ��̡����̴���Ϊ0x01��������Ϊ0x02��
        0x00,

        //iConfiguration�ֶΡ��ýӿڵ��ַ�������ֵ������û�У�Ϊ0��
        0x00,

        /******************HID������************************/
        //bLength�ֶΡ���HID��������ֻ��һ���¼������������Գ���Ϊ9�ֽڡ�
        0x09,

        //bDescriptorType�ֶΡ�HID�������ı��Ϊ0x21��
        0x21,

        //bcdHID�ֶΡ���Э��ʹ�õ�HID1.1Э�顣ע����ֽ����ȡ�
        0x10,
        0x01,

        //bCountyCode�ֶΡ��豸���õĹ��Ҵ��룬����ѡ��Ϊ����������0x21��
        0x21,

        //bNumDescriptors�ֶΡ��¼�����������Ŀ������ֻ��һ��������������
        0x01,

        //bDescritporType�ֶΡ��¼������������ͣ�Ϊ���������������Ϊ0x22��
        0x22,

        //bDescriptorLength�ֶΡ��¼��������ĳ��ȡ��¼�������Ϊ������������
        sizeof(ReportDescriptor) & 0xFF, (sizeof(ReportDescriptor) >> 8) & 0xFF,

        /**********************�˵�������***********************/
        //bLength�ֶΡ��˵�����������Ϊ7�ֽڡ�
        0x07,

        //bDescriptorType�ֶΡ��˵����������Ϊ0x05��
        0x05,

        //bEndpointAddress�ֶΡ��˵�ĵ�ַ������ʹ��D12������˵�1��
        //D7λ��ʾ���ݷ�������˵�D7Ϊ1����������˵�1�ĵ�ַΪ0x81��
        0x81,

        //bmAttributes�ֶΡ�D1~D0Ϊ�˵㴫������ѡ��
        //�ö˵�Ϊ�ж϶˵㡣�ж϶˵�ı��Ϊ3������λ����Ϊ0��
        0x03,

        //wMaxPacketSize�ֶΡ��ö˵�����������˵�1��������Ϊ16�ֽڡ�
        //ע����ֽ����ȡ�
        0x10,
        0x00,

        //bInterval�ֶΡ��˵��ѯ��ʱ�䣬��������Ϊ10��֡ʱ�䣬��10ms��
        0x01 };

#elif (USB_COMPLIE_MODE == USB_BULK)

uchar ConfigurationDescriptor[9 + 9 + 7 + 7] =
{
/***************����������***********************/
//bLength�ֶΡ������������ĳ���Ϊ9�ֽڡ�
        0x09,

        //bDescriptorType�ֶΡ��������������Ϊ0x02��
        0x02,

        //wTotalLength�ֶΡ��������������ϵ��ܳ��ȣ�
        //�������������������ӿ��������������������˵��������ȡ�
        sizeof(ConfigurationDescriptor) & 0xFF, //���ֽ�
        (sizeof(ConfigurationDescriptor) >> 8) & 0xFF, //���ֽ�

        //bNumInterfaces�ֶΡ������ð����Ľӿ�����ֻ��һ���ӿڡ�
        0x01,

        //bConfiguration�ֶΡ������õ�ֵΪ1��
        0x01,

        //iConfigurationz�ֶΣ������õ��ַ�������������û�У�Ϊ0��
        0x05,

        //bmAttributes�ֶΣ����豸�����ԡ��������ǵİ��������߹���ģ�
        //�������ǲ���ʵ��Զ�̻��ѵĹ��ܣ����Ը��ֶε�ֵΪ0x80��
        0xc0,

        //bMaxPower�ֶΣ����豸��Ҫ�������������������ǵİ���
        //��Ҫ�ĵ�������100mA�����������������Ϊ100mA������ÿ��λ
        //����Ϊ2mA��������������Ϊ50(0x32)��
        0xfa,

        /*******************�ӿ�������*********************/
        //bLength�ֶΡ��ӿ��������ĳ���Ϊ9�ֽڡ�
        0x09,

        //bDescriptorType�ֶΡ��ӿ��������ı��Ϊ0x04��
        0x04,

        //bInterfaceNumber�ֶΡ��ýӿڵı�ţ���һ���ӿڣ����Ϊ0��
        0x00,

        //bAlternateSetting�ֶΡ��ýӿڵı��ñ�ţ�Ϊ0��
        0x00,

        //bNumEndpoints�ֶΡ���0�˵����Ŀ������USB������ֻ��Ҫһ��
        //�ж�����˵㣬��˸�ֵΪ1��
        0x02,

        //bInterfaceClass�ֶΡ��ýӿ���ʹ�õ��ࡣUSB��������HID�࣬
        //HID��ı���Ϊ0x03��
        0xff,

        //bInterfaceSubClass�ֶΡ��ýӿ���ʹ�õ����ࡣ��HID1.1Э���У�
        //ֻ�涨��һ�����ࣺ֧��BIOS�������������ࡣ
        //USB���̡�������ڸ����࣬�������Ϊ0x01��
        0x00,

        //bInterfaceProtocol�ֶΡ��������Ϊ֧���������������࣬
        //��Э���ѡ�����ͼ��̡����̴���Ϊ0x01��������Ϊ0x02��
        0x00,

        //iConfiguration�ֶΡ��ýӿڵ��ַ�������ֵ������û�У�Ϊ0��
        0x04,

        /**********************�˵�������***********************/
        //bLength�ֶΡ��˵�����������Ϊ7�ֽڡ�
        0x07,

        //bDescriptorType�ֶΡ��˵����������Ϊ0x05��
        0x05,

        //bEndpointAddress�ֶΡ��˵�ĵ�ַ������ʹ��D12������˵�1��
        //D7λ��ʾ���ݷ�������˵�D7Ϊ1����������˵�1�ĵ�ַΪ0x81��
        0x81,

        //bmAttributes�ֶΡ�D1~D0Ϊ�˵㴫������ѡ��
        //�ö˵�Ϊ�ж϶˵㡣�ж϶˵�ı��Ϊ3������λ����Ϊ0��
        0x02,

		//wMaxPacketSize�ֶΡ��ö˵�����������˵�1��������Ϊ16�ֽڡ�
		//ע����ֽ����ȡ�
		0x40, 0x00,

        //bInterval�ֶΡ��˵��ѯ��ʱ�䣬��������Ϊ10��֡ʱ�䣬��10ms��
        0x01,
        //------------------out bulk-------------
         //bLength�ֶΡ��˵�����������Ϊ7�ֽڡ�
        0x07,

        //bDescriptorType�ֶΡ��˵����������Ϊ0x05��
        0x05,

        //bEndpointAddress�ֶΡ��˵�ĵ�ַ������ʹ��D12������˵�1��
        //D7λ��ʾ���ݷ�������˵�D7Ϊ1����������˵�1�ĵ�ַΪ0x81��
        0x02,

        //bmAttributes�ֶΡ�D1~D0Ϊ�˵㴫������ѡ��
        //�ö˵�Ϊ�ж϶˵㡣�ж϶˵�ı��Ϊ3������λ����Ϊ0��
        0x02,

		//wMaxPacketSize�ֶΡ��ö˵�����������˵�1��������Ϊ16�ֽڡ�
		//ע����ֽ����ȡ�
		0x40, 0x00,

		//bInterval�ֶΡ��˵��ѯ��ʱ�䣬��������Ϊ10��֡ʱ�䣬��10ms��
		0x01 };
#endif


extern sunxi_udc_t sunxi_udc_source;

static int __usb_get_descriptor(struct usb_device_request *req, uchar *buffer)
{
    int ret = SUNXI_USB_REQ_SUCCESSED;

    //��ȡ������
    switch (req->wValue >> 8)
    {
    case USB_DT_DEVICE:		//�豸������
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
		dev_dscrptr->bDeviceClass = 0x00;		//�豸�ࣺHID
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
		dev_dscrptr->bDeviceClass = 0xff;		//�豸�� bulk
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

    case USB_DT_CONFIG:		//����������
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
    //֪ͨ��ѭ��������д������
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
     if((fastboot_data_flag == 1) && ((char *)sunxi_ubuf->rx_req_buffer == all_download_bytes + trans_data.base_recv_buffer))	//�������
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
