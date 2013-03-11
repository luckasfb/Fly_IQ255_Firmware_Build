/* Copyright Statement: 
* 
* This software/firmware and related documentation ("MediaTek Software") are 
* protected under relevant copyright laws. The information contained herein 
* is confidential and proprietary to MediaTek Inc. and/or its licensors. 
* Without the prior written permission of MediaTek inc. and/or its licensors, 
* any reproduction, modification, use or disclosure of MediaTek Software, 
* and information contained herein, in whole or in part, shall be strictly prohibited. 
*/  
/* MediaTek Inc. (C) 2010. All rights reserved. 
* 
* BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES 
* THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE") 
* RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON 
* AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES, 
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF 
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT. 
* NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE 
* SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR 
* SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH 
* THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES 
* THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES 
* CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK 
* SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR 
* STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND 
* CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE, 
* AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE, 
* OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO 
* MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE. 
* 
* The following software/firmware and/or related documentation ("MediaTek Software") 
* have been modified by MediaTek Inc. All revisions are subject to any receiver's 
* applicable license agreements with MediaTek Inc. 
*/  
  
  
#include "tpd.h"  
#include <linux/interrupt.h>  
#include <cust_eint.h>  
#include <linux/i2c.h>  
#include <linux/sched.h>  
#include <linux/kthread.h>  
#include <linux/rtpm_prio.h>  
#include <linux/wait.h>  
#include <linux/time.h>  
#include <linux/delay.h>  
  
#include <linux/dma-mapping.h>  
#include <linux/slab.h>  
  
#include "cust_gpio_usage.h"  
#include "cust_gpio_usage.h"  
  
#include "Tpd_custom_msg2133.h"  
  
#ifdef MT6575  
#include <mach/mt6575_pm_ldo.h>  
#include <mach/mt6575_typedefs.h>  
#include <mach/mt6575_boot.h>  
#endif  
  
#define GPIO_CTP_MSG2133_EN_PIN          125 // GPIO_CTP_EN_PIN  
#define GPIO_CTP_MSG2133_EN_PIN_M_GPIO    GPIO_MODE_00  
  
#define GPIO_CTP_MSG2133_EINT_PIN         75 //  GPIO_CTP_EINT_PIN  
#define GPIO_CTP_MSG2133_EINT_PIN_M_GPIO   GPIO_CTP_EINT_PIN_M_EINT  
  
extern struct tpd_device *tpd;  
  
struct i2c_client *i2c_client = NULL;  
struct task_struct *mthread = NULL;  
  
struct TpdTouchDataT  
{  
    U8 packet_id;  
    U8 x_h_y_h;  
    U8 x_l;  
    U8 y_l;  
    U8 disx_h_disy_h;  
    U8 disx_l;  
    U8 disy_l;  
    U8 checksum;  
};  
static struct TpdTouchDataT TpdTouchData;  
  
static DECLARE_WAIT_QUEUE_HEAD(waiter);  
  
  
static void tpd_eint_interrupt_handler(void);  
  
  
extern void mt65xx_eint_unmask(unsigned int line);  
extern void mt65xx_eint_mask(unsigned int line);  
extern void mt65xx_eint_set_hw_debounce(kal_uint8 eintno, kal_uint32 ms);  
extern kal_uint32 mt65xx_eint_set_sens(kal_uint8 eintno, kal_bool sens);  
extern void mt65xx_eint_registration(kal_uint8 eintno, kal_bool Dbounce_En,  
                                     kal_bool ACT_Polarity, void (EINT_FUNC_PTR)(void),  
                                     kal_bool auto_umask);  
  
  
static int __devinit tpd_probe(struct i2c_client *client, const struct i2c_device_id *id);  
static int tpd_detect(struct i2c_client *client, int kind, struct i2c_board_info *info);  
static int __devexit tpd_remove(struct i2c_client *client);  
static int touch_event_handler(void *unused);  
  
static int tpd_flag = 0;  
static int point_num = 0;  
static int p_point_num = 0;  
  
#ifdef TPD_HAVE_BUTTON  
static int tpd_keys_local[TPD_KEY_COUNT] = TPD_KEYS;  
static int tpd_keys_dim_local[TPD_KEY_COUNT][4] = TPD_KEYS_DIM;  
#endif  
  
//#define TPD_CLOSE_POWER_IN_SLEEP  
//#define TP_DEBUG  
#define TP_FIRMWARE_UPDATE  
#define TP_PROXIMITY_SENSOR  
#define TPD_OK 0  
// debug macros  
  
#define TPD_LOGI  printk  
#define TPD_LOGV printk  
#if defined(TP_DEBUG_MSG)  
#define pr_tp(format, args...) printk("<MSG>" format, ##args)  
#define pr_ch(format, args...)                      \  
    printk("<MSG>" "%s <%d>,%s(),cheehwa_print:\n\t"  \  
    format,__FILE__,__LINE__,__func__, ##arg)  
#else  
#define pr_tp(format, args...)  do {} while (0)  
#define pr_ch(format, args...)  do {} while (0)  
#define pr_k(format, args...)  do {} while (0)  
#endif  
  
#ifdef TP_PROXIMITY_SENSOR  
char ps_data_state[1] = {0};  
enum  
{  
    DISABLE_CTP_PS,  
    ENABLE_CTP_PS,  
    RESET_CTP_PS  
};  
#endif  
  
struct TouchInfoT  
{  
    int x1, y1;  
    int x2, y2;  
    int pressure;  
    int count;  
#ifdef TPD_HAVE_BUTTON   
    int key_id;  
    int key_value;  
#endif  
};  
  
#ifdef TP_FIRMWARE_UPDATE  
#define U8 unsigned char  
#define S8 signed char  
#define U16 unsigned short  
#define S16 signed short  
#define U32 unsigned int  
#define S32 signed int  
#define TOUCH_ADDR_MSG21XX   0x4C  
#define FW_ADDR_MSG21XX      0xC4  
#define FW_UPDATE_ADDR_MSG21XX   0x92  
static  char *fw_version;  
#define DWIIC_MODE_ISP    0  
#define DWIIC_MODE_DBBUS  1  
#define   DOWNLOAD_FIRMWARE_BUF_SIZE   59*1024  
static U8 *download_firmware_buf = NULL;  
static int FwDataCnt=0;  
//static int FwVersion;  
struct class *firmware_class;  
struct device *firmware_cmd_dev;  
  
static int update_switch = 0;  
#define ENABLE_DMA      0  
#if ENABLE_DMA  
static u8 *gpDMABuf_va = NULL;  
static u32 gpDMABuf_pa = NULL;  
#endif  
  
#endif  
  
struct touch_info  
{  
    unsigned short y[3];  
    unsigned short x[3];  
    unsigned short p[3];  
    unsigned short count;  
};  
  
typedef struct  
{  
    unsigned short pos_x;  
    unsigned short pos_y;  
    unsigned short pos_x2;  
    unsigned short pos_y2;  
    unsigned short temp2;  
    unsigned short temp;  
    short dst_x;  
    short dst_y;  
    unsigned char checksum;  
} SHORT_TOUCH_STATE;  
  
static const struct i2c_device_id tpd_id[] = {{"msg2133", 0}, {}};  
unsigned short myforce[] = {0, TOUCH_ADDR_MSG21XX, I2C_CLIENT_END, I2C_CLIENT_END};  
static const unsigned short *const forces[] = { myforce, NULL };  
static struct i2c_client_address_data addr_data = { .forces = forces, };  
  
#ifdef TP_FIRMWARE_UPDATE  
static void i2c_write(u8 addr, u8 *pbt_buf, int dw_lenth)  
{  
    int ret;  
    i2c_client->addr = addr;  
    i2c_client->addr|=I2C_ENEXT_FLAG;  
    ret = i2c_master_send(i2c_client, pbt_buf, dw_lenth);  
    i2c_client->addr = TOUCH_ADDR_MSG21XX;  
    i2c_client->addr|=I2C_ENEXT_FLAG;  
  
    if(ret <= 0)  
    {  
        printk("i2c_write_interface error line = %d, ret = %d\n", __LINE__, ret);  
    }  
}  
  
static void i2c_read(u8 addr, u8 *pbt_buf, int dw_lenth)  
{  
    int ret;  
    i2c_client->addr = addr;  
    i2c_client->addr|=I2C_ENEXT_FLAG;  
    ret = i2c_master_recv(i2c_client, pbt_buf, dw_lenth);  
    i2c_client->addr = TOUCH_ADDR_MSG21XX;  
    i2c_client->addr|=I2C_ENEXT_FLAG;  
  
    if(ret <= 0)  
    {  
        printk("i2c_read_interface error line = %d, ret = %d\n", __LINE__, ret);  
    }  
}  
  
static void i2c_write_update_msg2133(u8 *pbt_buf, int dw_lenth)  
{  
    int ret;  
    i2c_client->addr = FW_UPDATE_ADDR_MSG21XX;  
    i2c_client->addr|=I2C_ENEXT_FLAG;  
    ret = i2c_master_send(i2c_client, pbt_buf, dw_lenth);  
    i2c_client->addr = TOUCH_ADDR_MSG21XX;  
    i2c_client->addr|=I2C_ENEXT_FLAG;  
  
    // ret = i2c_smbus_write_i2c_block_data(i2c_client, *pbt_buf, dw_lenth-1, pbt_buf+1);  
    if(ret <= 0)  
    {  
        printk("i2c_write_interface error line = %d, ret = %d\n", __LINE__, ret);  
    }  
}  
  
static void i2c_write_msg2133(u8 *pbt_buf, int dw_lenth)  
{  
    int ret;  
    i2c_client->timing = 40;  
    i2c_client->addr = FW_ADDR_MSG21XX;  
    i2c_client->addr|=I2C_ENEXT_FLAG;  
    ret = i2c_master_send(i2c_client, pbt_buf, dw_lenth);  
    i2c_client->addr = TOUCH_ADDR_MSG21XX;  
    i2c_client->addr|=I2C_ENEXT_FLAG;  
    i2c_client->timing = 240;  
  
    // ret = i2c_smbus_write_i2c_block_data(i2c_client, *pbt_buf, dw_lenth-1, pbt_buf+1);  
    if(ret <= 0)  
    {  
        printk("i2c_write_interface error line = %d, ret = %d\n", __LINE__, ret);  
    }  
}  
static void i2c_read_update_msg2133(u8 *pbt_buf, int dw_lenth)  
{  
    int ret;  
    i2c_client->addr = FW_UPDATE_ADDR_MSG21XX;  
    i2c_client->addr|=I2C_ENEXT_FLAG;  
    ret = i2c_master_recv(i2c_client, pbt_buf, dw_lenth);  
    i2c_client->addr = TOUCH_ADDR_MSG21XX;  
    i2c_client->addr|=I2C_ENEXT_FLAG;  
    //  ret=i2c_smbus_read_i2c_block_data(i2c_client, *pbt_buf, dw_lenth-1, pbt_buf+1);  
  
    if(ret <= 0)  
    {  
        printk("i2c_read_interface error line = %d, ret = %d\n", __LINE__, ret);  
    }  
}  
  
static void i2c_read_msg2133(u8 *pbt_buf, int dw_lenth)  
{  
    int ret;  
    i2c_client->timing = 40;  
    i2c_client->addr = FW_ADDR_MSG21XX;  
    i2c_client->addr|=I2C_ENEXT_FLAG;  
    ret = i2c_master_recv(i2c_client, pbt_buf, dw_lenth);  
    i2c_client->addr = TOUCH_ADDR_MSG21XX;  
    i2c_client->addr|=I2C_ENEXT_FLAG;  
    i2c_client->timing = 240;  
    //  ret=i2c_smbus_read_i2c_block_data(i2c_client, *pbt_buf, dw_lenth-1, pbt_buf+1);  
  
    if(ret <= 0)  
    {  
        printk("i2c_read_interface error line = %d, ret = %d\n", __LINE__, ret);  
    }  
}  
  
#if ENABLE_DMA  
ssize_t msg2133_dma_read_m_byte(u8 *returnData_va, u32 returnData_pa, U16 len)  
{  
    char     readData = 0;  
    int     ret = 0, read_count = 0, read_length = 0;  
    int    i, total_count = len;  
  
    if(len == 0)  
    {  
        printk("[Error]msg2133_dma_read Read Len should not be zero!! \n");  
        return 0;  
    }  
  
    //gpDMABuf_va = returnData_va; //mtk  
    i2c_client->addr = FW_UPDATE_ADDR_MSG21XX;  
    i2c_client->addr |= I2C_DMA_FLAG;  
    i2c_client->addr|=I2C_ENEXT_FLAG;  
    returnData_va[0] = 0x11;  
    ret = i2c_master_send(i2c_client, returnData_pa, 1);  
  
    if(ret < 0)  
    {  
        printk("[Error]MATV sends command error!! \n");  
        i2c_client->addr = TOUCH_ADDR_MSG21XX;  
        i2c_client->addr|=I2C_ENEXT_FLAG;  
        return 0;  
    }  
  
    ret = i2c_master_recv(i2c_client, returnData_pa, len); // mtk  
    i2c_client->addr = TOUCH_ADDR_MSG21XX;  
    i2c_client->addr|=I2C_ENEXT_FLAG;  
  
    if(ret < 0)  
    {  
        printk("[Error]msg2133_dma_read reads data error!! \n");  
        return 0;  
    }  
  
    //for (i = 0; i< total_count; i++)  
    //    MATV_LOGD("[MATV]I2C ReadData[%d] = %x\n",i,returnData_va[i]);  
    return 1;  
}  
  
#define MAX_CMD_LEN 255  
ssize_t msg2133_dma_write_m_byte(u8 *Data, U16 len)  
{  
    char addr_bak;  
    u32 phyAddr = 0;  
    u8 *buf_dma = NULL;  
    u32 old_addr = 0;  
    int ret = 0;  
    int retry = 3;  
  
    addr_bak = i2c_client->addr;  
    i2c_client->addr = FW_UPDATE_ADDR_MSG21XX;  
    i2c_client->addr |= I2C_ENEXT_FLAG;  
    if (len > MAX_CMD_LEN) {  
        //TPD_LOGI("[i2c_master_send_ext] exceed the max write length \n");  
        return -1;  
    }  
  
    phyAddr = 0;  
    buf_dma = dma_alloc_coherent(0, len, &phyAddr, GFP_KERNEL);  
    if (NULL == buf_dma) {  
        //TPD_LOGI("[i2c_master_send_ext] Not enough memory \n");  
        return -1;  
    }  
    memcpy(buf_dma, Data, len);  
    i2c_client->addr |= I2C_DMA_FLAG;  
    i2c_client->addr|=I2C_ENEXT_FLAG;  
  
    do {  
        ret = i2c_master_send(i2c_client, (u8*)phyAddr, len);  
        retry --;  
        if (ret != len) {  
            //TPD_LOGI("[i2c_master_send_ext] Error sent I2C ret = %d\n", ret);  
        }  
    }while ((ret != len) && (retry > 0));  
  
    dma_free_coherent(0, len, buf_dma, phyAddr);  
  
    i2c_client->addr = addr_bak;  
  
    return 1;  
}  
  
U8 drvISP_DMA_Read(U8 *pDataToRead, U32 pa_addr, U8 n)    //First it needs send 0x11 to notify we want to get flash data back.  
{  
    //    U8 Read_cmd = 0x11;  
    //    unsigned char dbbus_rx_data[2] = {0};  
    //    i2c_write_update_msg2133( &Read_cmd, 1);  
    msg2133_dma_read_m_byte(pDataToRead, pa_addr, n);  
    return 0;  
}  
  
#endif  
  
void Get_Chip_Version(void)  
{  
    unsigned char dbbus_tx_data[3];  
    unsigned char dbbus_rx_data[2];  
    dbbus_tx_data[0] = 0x10;  
    dbbus_tx_data[1] = 0x1E;  
    dbbus_tx_data[2] = 0xCE;  
    i2c_write_msg2133(&dbbus_tx_data[0], 3);  
    i2c_read_msg2133(&dbbus_rx_data[0], 2);  
  
    if(dbbus_rx_data[1] == 0)  
    {  
        // it is Catch2  
        pr_tp("*** Catch2 ***\n");  
        //FwVersion  = 2;// 2 means Catch2  
    }  
    else  
    {  
        // it is catch1  
        pr_tp("*** Catch1 ***\n");  
        //FwVersion  = 1;// 1 means Catch1  
    }  
}  
  
void dbbusDWIICEnterSerialDebugMode(void)  
{  
    U8 data[5];  
    // Enter the Serial Debug Mode  
    data[0] = 0x53;  
    data[1] = 0x45;  
    data[2] = 0x52;  
    data[3] = 0x44;  
    data[4] = 0x42;  
    i2c_write_msg2133(data, 5);  
}  
  
void dbbusDWIICStopMCU(void)  
{  
    U8 data[1];  
    // Stop the MCU  
    data[0] = 0x37;  
    i2c_write_msg2133(data, 1);  
}  
  
void dbbusDWIICIICUseBus(void)  
{  
    U8 data[1];  
    // IIC Use Bus  
    data[0] = 0x35;  
    i2c_write_msg2133(data, 1);  
}  
  
void dbbusDWIICIICReshape(void)  
{  
    U8 data[1];  
    // IIC Re-shape  
    data[0] = 0x71;  
    i2c_write_msg2133(data, 1);  
}  
  
void dbbusDWIICIICNotUseBus(void)  
{  
    U8 data[1];  
    // IIC Not Use Bus  
    data[0] = 0x34;  
    i2c_write_msg2133(data, 1);  
}  
  
void dbbusDWIICNotStopMCU(void)  
{  
    U8 data[1];  
    // Not Stop the MCU  
    data[0] = 0x36;  
    i2c_write_msg2133(data, 1);  
}  
  
void dbbusDWIICExitSerialDebugMode(void)  
{  
    U8 data[1];  
    // Exit the Serial Debug Mode  
    data[0] = 0x45;  
    i2c_write_msg2133(data, 1);  
    // Delay some interval to guard the next transaction  
    //udelay ( 200 );        // delay about 0.2ms  
}  
  
void drvISP_EntryIspMode(void)  
{  
    U8 bWriteData[5] =  
    {  
        0x4D, 0x53, 0x54, 0x41, 0x52  
    };  
    i2c_write_update_msg2133(bWriteData, 5);  
    mdelay(10);           // delay about 1ms  
}  
  
U8 drvISP_Read(U8 n, U8 *pDataToRead)    //First it needs send 0x11 to notify we want to get flash data back.  
{  
    U8 Read_cmd = 0x11;  
    U8 i = 0;  
    unsigned char dbbus_rx_data[16] = {0};  
    i2c_write_update_msg2133(&Read_cmd, 1);  
    //if (n == 1)  
    {  
        i2c_read_update_msg2133(&dbbus_rx_data[0], n + 1);  
  
        for(i = 0; i < n; i++)  
        {  
            *(pDataToRead + i) = dbbus_rx_data[i + 1];  
        }  
    }  
    //else  
    {  
        //     i2c_read_update_msg2133(pDataToRead, n);  
    }  
    return 0;  
}  
  
void drvISP_WriteEnable(void)  
{  
    U8 bWriteData[2] =  
    {  
        0x10, 0x06  
    };  
    U8 bWriteData1 = 0x12;  
    i2c_write_update_msg2133(bWriteData, 2);  
    i2c_write_update_msg2133(&bWriteData1, 1);  
}  
  
  
void drvISP_ExitIspMode(void)  
{  
    U8 bWriteData = 0x24;  
    i2c_write_update_msg2133(&bWriteData, 1);  
}  
  
U8 drvISP_ReadStatus(void)  
{  
    U8 bReadData = 0;  
    U8 bWriteData[2] =  
    {  
        0x10, 0x05  
    };  
    U8 bWriteData1 = 0x12;  
    i2c_write_update_msg2133(bWriteData, 2);  
    drvISP_Read(1, &bReadData);  
    i2c_write_update_msg2133(&bWriteData1, 1);  
    return bReadData;  
}  
  
  
void drvISP_BlockErase(U32 addr)  
{  
    U8 bWriteData[5] = { 0x00, 0x00, 0x00, 0x00, 0x00 };  
    U8 bWriteData1 = 0x12;  
    pr_ch("The drvISP_ReadStatus0=%d\n", drvISP_ReadStatus());  
    drvISP_WriteEnable();  
    pr_ch("The drvISP_ReadStatus1=%d\n", drvISP_ReadStatus());  
    //Enable write status register  
    bWriteData[0] = 0x10;  
    bWriteData[1] = 0x50;  
    i2c_write_update_msg2133(bWriteData, 2);  
    i2c_write_update_msg2133(&bWriteData1, 1);  
    //Write Status  
    bWriteData[0] = 0x10;  
    bWriteData[1] = 0x01;  
    bWriteData[2] = 0x00;  
    i2c_write_update_msg2133(bWriteData, 3);  
    i2c_write_update_msg2133(&bWriteData1, 1);  
    //Write disable  
    bWriteData[0] = 0x10;  
    bWriteData[1] = 0x04;  
    i2c_write_update_msg2133(bWriteData, 2);  
    i2c_write_update_msg2133(&bWriteData1, 1);  
  
    while((drvISP_ReadStatus() & 0x01) == 0x01)  
    {  
        ;  
    }  
  
    pr_ch("The drvISP_ReadStatus3=%d\n", drvISP_ReadStatus());  
    drvISP_WriteEnable();  
    pr_ch("The drvISP_ReadStatus4=%d\n", drvISP_ReadStatus());  
    bWriteData[0] = 0x10;  
    bWriteData[1] = 0xC7;        //Block Erase  
    //bWriteData[2] = ((addr >> 16) & 0xFF) ;  
    //bWriteData[3] = ((addr >> 8) & 0xFF) ;  
    // bWriteData[4] = (addr & 0xFF) ;  
    i2c_write_update_msg2133(bWriteData, 2);  
    //i2c_write_update_msg2133( &bWriteData, 5);  
    i2c_write_update_msg2133(&bWriteData1, 1);  
  
    while((drvISP_ReadStatus() & 0x01) == 0x01)  
    {  
        ;  
    }  
}  
  
void drvISP_Program(U16 k, U8 *pDataToWrite)  
{  
    U16 i = 0;  
    U16 j = 0;  
    //U16 n = 0;  
    U8 TX_data[133];  
    U8 bWriteData1 = 0x12;  
    U32 addr = k * 1024;  
#if ENABLE_DMA  
  
    for(j = 0; j < 8; j++)    //128*8 cycle  
    {  
        TX_data[0] = 0x10;  
        TX_data[1] = 0x02;// Page Program CMD  
        TX_data[2] = (addr + 128 * j) >> 16;  
        TX_data[3] = (addr + 128 * j) >> 8;  
        TX_data[4] = (addr + 128 * j);  
  
        for(i = 0; i < 128; i++)  
        {  
            TX_data[5 + i] = pDataToWrite[j * 128 + i];  
        }  
  
        while((drvISP_ReadStatus() & 0x01) == 0x01)  
        {  
            ;    //wait until not in write operation  
        }  
  
        drvISP_WriteEnable();  
        //        i2c_write_update_msg2133( TX_data, 133);   //write 133 byte per cycle  
        msg2133_dma_write_m_byte(TX_data, 133);  
        i2c_write_update_msg2133(&bWriteData1, 1);  
    }  
  
#else  
  
    for(j = 0; j < 512; j++)    //128*8 cycle  
    {  
        TX_data[0] = 0x10;  
        TX_data[1] = 0x02;// Page Program CMD  
        TX_data[2] = (addr + 2 * j) >> 16;  
        TX_data[3] = (addr + 2 * j) >> 8;  
        TX_data[4] = (addr + 2 * j);  
  
        for(i = 0; i < 2; i++)  
        {  
            TX_data[5 + i] = pDataToWrite[j * 2 + i];  
        }  
  
        while((drvISP_ReadStatus() & 0x01) == 0x01)  
        {  
            ;    //wait until not in write operation  
        }  
  
        drvISP_WriteEnable();  
        i2c_write_update_msg2133(TX_data, 7);    //write 133 byte per cycle  
        i2c_write_update_msg2133(&bWriteData1, 1);  
    }  
  
#endif  
}  
  
  
void drvISP_Verify(U16 k, U8 *pDataToVerify)  
{  
    U16 i = 0, j = 0;  
    U8 bWriteData[5] =  
    {  
        0x10, 0x03, 0, 0, 0  
    };  
    U8 bWriteData1 = 0x12;  
    U32 addr = k * 1024;  
    U8 index = 0;  
#if ENABLE_DMA  
    U8 *RX_data = gpDMABuf_va; //mtk  
  
    for(j = 0; j < 8; j++)    //128*8 cycle  
    {  
        bWriteData[2] = (U8)((addr + j * 128) >> 16);  
        bWriteData[3] = (U8)((addr + j * 128) >> 8);  
        bWriteData[4] = (U8)(addr + j * 128);  
  
        while((drvISP_ReadStatus() & 0x01) == 0x01)  
        {  
            ;    //wait until not in write operation  
        }  
  
        i2c_write_update_msg2133(bWriteData, 5);     //write read flash addr  
        drvISP_DMA_Read(gpDMABuf_va, gpDMABuf_pa, 128); //mtk  
        i2c_write_update_msg2133(&bWriteData1, 1);     //cmd end  
  
        for(i = 0; i < 128; i++)    //log out if verify error  
        {  
            if((RX_data[i] != 0) && index < 10)  
            {  
                pr_tp("j=%d,RX_data[%d]=0x%x\n", j, i, RX_data[i]);  
                index++;  
            }  
  
            if(RX_data[i] != pDataToVerify[128 * j + i])  
            {  
                pr_tp("k=%d,j=%d,i=%d===============Update Firmware Error================", k, j, i);  
            }  
        }  
    }  
  
#else  
    U8 RX_data[128];  
  
    for(j = 0; j < 256; j++)    //128*8 cycle  
    {  
        bWriteData[2] = (U8)((addr + j * 4) >> 16);  
        bWriteData[3] = (U8)((addr + j * 4) >> 8);  
        bWriteData[4] = (U8)(addr + j * 4);  
  
        while((drvISP_ReadStatus() & 0x01) == 0x01)  
        {  
            ;    //wait until not in write operation  
        }  
  
        i2c_write_update_msg2133(bWriteData, 5);     //write read flash addr  
        drvISP_Read(4, RX_data);  
        i2c_write_update_msg2133(&bWriteData1, 1);     //cmd end  
  
        for(i = 0; i < 4; i++)    //log out if verify error  
        {  
            if((RX_data[i] != 0) && index < 10)  
            {  
                pr_tp("j=%d,RX_data[%d]=0x%x\n", j, i, RX_data[i]);  
                index++;  
            }  
  
            if(RX_data[i] != pDataToVerify[4 * j + i])  
            {  
                pr_tp("k=%d,j=%d,RX_data[%d]=0x%x===============Update Firmware Error================", k, j, i, RX_data[i]);  
            }  
        }  
    }  
  
#endif  
}  
  
static ssize_t firmware_update_show(struct device *dev,  
struct device_attribute *attr, char *buf)  
{  
    return sprintf(buf, "%s\n", fw_version);  
}  
  
static ssize_t firmware_update_store(struct device *dev,  
struct device_attribute *attr, const char *buf, size_t size)  
{  
    U8 i;  
    U8 dbbus_tx_data[4];  
    unsigned char dbbus_rx_data[2] = {0};  
    update_switch = 1;  
    pr_ch("\n");  
    //drvISP_EntryIspMode();  
    //drvISP_BlockErase(0x00000);  
    //M by cheehwa _HalTscrHWReset();  
    mt_set_gpio_mode(GPIO_CTP_MSG2133_EN_PIN, GPIO_CTP_MSG2133_EN_PIN_M_GPIO);  
    mt_set_gpio_dir(GPIO_CTP_MSG2133_EN_PIN, GPIO_DIR_OUT);  
    mt_set_gpio_out(GPIO_CTP_MSG2133_EN_PIN, GPIO_OUT_ZERO);  
    msleep(100);  
    mt_set_gpio_mode(GPIO_CTP_MSG2133_EN_PIN, GPIO_CTP_MSG2133_EN_PIN_M_GPIO);  
    mt_set_gpio_dir(GPIO_CTP_MSG2133_EN_PIN, GPIO_DIR_OUT);  
    mt_set_gpio_out(GPIO_CTP_MSG2133_EN_PIN, GPIO_OUT_ONE);  
    mdelay(500);  
    //msctpc_LoopDelay ( 100 );        // delay about 100ms*****  
    // Enable slave's ISP ECO mode  
    dbbusDWIICEnterSerialDebugMode();  
    dbbusDWIICStopMCU();  
    dbbusDWIICIICUseBus();  
    dbbusDWIICIICReshape();  
    pr_ch("dbbusDWIICIICReshape\n");  
    dbbus_tx_data[0] = 0x10;  
    dbbus_tx_data[1] = 0x08;  
    dbbus_tx_data[2] = 0x0c;  
    dbbus_tx_data[3] = 0x08;  
    // Disable the Watchdog  
    i2c_write_msg2133(dbbus_tx_data, 4);  
    //Get_Chip_Version();  
    dbbus_tx_data[0] = 0x10;  
    dbbus_tx_data[1] = 0x11;  
    dbbus_tx_data[2] = 0xE2;  
    dbbus_tx_data[3] = 0x00;  
    i2c_write_msg2133(dbbus_tx_data, 4);  
    dbbus_tx_data[0] = 0x10;  
    dbbus_tx_data[1] = 0x3C;  
    dbbus_tx_data[2] = 0x60;  
    dbbus_tx_data[3] = 0x55;  
    i2c_write_msg2133(dbbus_tx_data, 4);  
    pr_ch("update\n");  
    dbbus_tx_data[0] = 0x10;  
    dbbus_tx_data[1] = 0x3C;  
    dbbus_tx_data[2] = 0x61;  
    dbbus_tx_data[3] = 0xAA;  
    i2c_write_msg2133(dbbus_tx_data, 4);  
    //Stop MCU  
    dbbus_tx_data[0] = 0x10;  
    dbbus_tx_data[1] = 0x0F;  
    dbbus_tx_data[2] = 0xE6;  
    dbbus_tx_data[3] = 0x01;  
    i2c_write_msg2133(dbbus_tx_data, 4);  
    //Enable SPI Pad  
    dbbus_tx_data[0] = 0x10;  
    dbbus_tx_data[1] = 0x1E;  
    dbbus_tx_data[2] = 0x02;  
    i2c_write_msg2133(dbbus_tx_data, 3);  
    i2c_read_msg2133(&dbbus_rx_data[0], 2);  
    pr_tp("dbbus_rx_data[0]=0x%x", dbbus_rx_data[0]);  
    dbbus_tx_data[3] = (dbbus_rx_data[0] | 0x20);  //Set Bit 5  
    i2c_write_msg2133(dbbus_tx_data, 4);  
    dbbus_tx_data[0] = 0x10;  
    dbbus_tx_data[1] = 0x1E;  
    dbbus_tx_data[2] = 0x25;  
    i2c_write_msg2133(dbbus_tx_data, 3);  
    dbbus_rx_data[0] = 0;  
    dbbus_rx_data[1] = 0;  
    i2c_read_msg2133(&dbbus_rx_data[0], 2);  
    pr_tp("dbbus_rx_data[0]=0x%x", dbbus_rx_data[0]);  
    dbbus_tx_data[3] = dbbus_rx_data[0] & 0xFC;  //Clear Bit 1,0  
    i2c_write_msg2133(dbbus_tx_data, 4);  
    /* 
    //------------ 
    // ISP Speed Change to 400K 
    dbbus_tx_data[0] = 0x10; 
    dbbus_tx_data[1] = 0x11; 
    dbbus_tx_data[2] = 0xE2; 
    i2c_write_msg2133( dbbus_tx_data, 3); 
    i2c_read_msg2133( &dbbus_rx_data[3], 1); 
    //pr_tp("dbbus_rx_data[0]=0x%x", dbbus_rx_data[0]); 
    dbbus_tx_data[3] = dbbus_tx_data[3]&0xf7;  //Clear Bit3 
    i2c_write_msg2133( dbbus_tx_data, 4); 
    */  
    //WP overwrite  
    dbbus_tx_data[0] = 0x10;  
    dbbus_tx_data[1] = 0x1E;  
    dbbus_tx_data[2] = 0x0E;  
    dbbus_tx_data[3] = 0x02;  
    i2c_write_msg2133(dbbus_tx_data, 4);  
    //set pin high  
    dbbus_tx_data[0] = 0x10;  
    dbbus_tx_data[1] = 0x1E;  
    dbbus_tx_data[2] = 0x10;  
    dbbus_tx_data[3] = 0x08;  
    i2c_write_msg2133(dbbus_tx_data, 4);  
    dbbusDWIICIICNotUseBus();  
    dbbusDWIICNotStopMCU();  
    dbbusDWIICExitSerialDebugMode();  
    ///////////////////////////////////////  
    // Start to load firmware  
    ///////////////////////////////////////  
    drvISP_EntryIspMode();  
    pr_ch("entryisp\n");  
    drvISP_BlockErase(0x00000);  
    //msleep(1000);  
    pr_tp("FwVersion=2");  
  
    for(i = 0; i < 59; i++)    // total  94 KB : 1 byte per R/W  
    {  
  
        pr_ch("drvISP_Program\n");  
        if (download_firmware_buf == NULL)  
            return 0;  
        drvISP_Program(i,&download_firmware_buf[i*1024]);  
        //pr_ch("drvISP_Verify\n");  
        //drvISP_Verify ( i,&download_firmware_buf[i*1024] ); //verify data  
    }  
  
    pr_tp("update OK\n");  
    drvISP_ExitIspMode();  
    FwDataCnt = 0;  
    if (download_firmware_buf != NULL)  
    {  
        kfree(download_firmware_buf);  
        download_firmware_buf = NULL;  
    }  
  
    mt_set_gpio_mode(GPIO_CTP_MSG2133_EN_PIN, GPIO_CTP_MSG2133_EN_PIN_M_GPIO);  
    mt_set_gpio_dir(GPIO_CTP_MSG2133_EN_PIN, GPIO_DIR_OUT);  
    mt_set_gpio_out(GPIO_CTP_MSG2133_EN_PIN, GPIO_OUT_ZERO);  
    msleep(100);  
    mt_set_gpio_mode(GPIO_CTP_MSG2133_EN_PIN, GPIO_CTP_MSG2133_EN_PIN_M_GPIO);  
    mt_set_gpio_dir(GPIO_CTP_MSG2133_EN_PIN, GPIO_DIR_OUT);  
    mt_set_gpio_out(GPIO_CTP_MSG2133_EN_PIN, GPIO_OUT_ONE);  
    msleep(500);  
    update_switch = 0;  
    return size;  
}  
  
static DEVICE_ATTR(update, 0777, firmware_update_show, firmware_update_store);  
  
/*test=================*/  
static ssize_t firmware_clear_show(struct device *dev,  
struct device_attribute *attr, char *buf)  
{  
    U16 k = 0, i = 0, j = 0;  
    U8 bWriteData[5] =  
    {  
        0x10, 0x03, 0, 0, 0  
    };  
    U8 RX_data[256];  
    U8 bWriteData1 = 0x12;  
    U32 addr = 0;  
    pr_ch("\n");  
  
    for(k = 0; k < 94; i++)    // total  94 KB : 1 byte per R/W  
    {  
        addr = k * 1024;  
  
        for(j = 0; j < 8; j++)    //128*8 cycle  
        {  
            bWriteData[2] = (U8)((addr + j * 128) >> 16);  
            bWriteData[3] = (U8)((addr + j * 128) >> 8);  
            bWriteData[4] = (U8)(addr + j * 128);  
  
            while((drvISP_ReadStatus() & 0x01) == 0x01)  
            {  
                ;    //wait until not in write operation  
            }  
  
            i2c_write_update_msg2133(bWriteData, 5);     //write read flash addr  
            drvISP_Read(128, RX_data);  
            i2c_write_update_msg2133(&bWriteData1, 1);    //cmd end  
  
            for(i = 0; i < 128; i++)    //log out if verify error  
            {  
                if(RX_data[i] != 0xFF)  
                {  
                    pr_tp("k=%d,j=%d,i=%d===============erase not clean================", k, j, i);  
                }  
            }  
        }  
    }  
  
    pr_tp("read finish\n");  
    return sprintf(buf, "%s\n", fw_version);  
}  
  
static ssize_t firmware_clear_store(struct device *dev,  
struct device_attribute *attr, const char *buf, size_t size)  
{  
    U8 dbbus_tx_data[4];  
    unsigned char dbbus_rx_data[2] = {0};  
    //msctpc_LoopDelay ( 100 );        // delay about 100ms*****  
    // Enable slave's ISP ECO mode  
    /* 
    dbbusDWIICEnterSerialDebugMode(); 
    dbbusDWIICStopMCU(); 
    dbbusDWIICIICUseBus(); 
    dbbusDWIICIICReshape();*/  
  
    dbbus_tx_data[0] = 0x10;  
    dbbus_tx_data[1] = 0x08;  
    dbbus_tx_data[2] = 0x0c;  
    dbbus_tx_data[3] = 0x08;  
    // Disable the Watchdog  
    i2c_write_msg2133(dbbus_tx_data, 4);  
    //Get_Chip_Version();  
    //FwVersion  = 2;  
    //if (FwVersion  == 2)  
    {  
        dbbus_tx_data[0] = 0x10;  
        dbbus_tx_data[1] = 0x11;  
        dbbus_tx_data[2] = 0xE2;  
        dbbus_tx_data[3] = 0x00;  
        i2c_write_msg2133(dbbus_tx_data, 4);  
    }  
    dbbus_tx_data[0] = 0x10;  
    dbbus_tx_data[1] = 0x3C;  
    dbbus_tx_data[2] = 0x60;  
    dbbus_tx_data[3] = 0x55;  
    i2c_write_msg2133(dbbus_tx_data, 4);  
    dbbus_tx_data[0] = 0x10;  
    dbbus_tx_data[1] = 0x3C;  
    dbbus_tx_data[2] = 0x61;  
    dbbus_tx_data[3] = 0xAA;  
    i2c_write_msg2133(dbbus_tx_data, 4);  
    //Stop MCU  
    dbbus_tx_data[0] = 0x10;  
    dbbus_tx_data[1] = 0x0F;  
    dbbus_tx_data[2] = 0xE6;  
    dbbus_tx_data[3] = 0x01;  
    i2c_write_msg2133(dbbus_tx_data, 4);  
    //Enable SPI Pad  
    dbbus_tx_data[0] = 0x10;  
    dbbus_tx_data[1] = 0x1E;  
    dbbus_tx_data[2] = 0x02;  
    i2c_write_msg2133(dbbus_tx_data, 3);  
    i2c_read_msg2133(&dbbus_rx_data[0], 2);  
    pr_tp("dbbus_rx_data[0]=0x%x", dbbus_rx_data[0]);  
    dbbus_tx_data[3] = (dbbus_rx_data[0] | 0x20);  //Set Bit 5  
    i2c_write_msg2133(dbbus_tx_data, 4);  
    dbbus_tx_data[0] = 0x10;  
    dbbus_tx_data[1] = 0x1E;  
    dbbus_tx_data[2] = 0x25;  
    i2c_write_msg2133(dbbus_tx_data, 3);  
    dbbus_rx_data[0] = 0;  
    dbbus_rx_data[1] = 0;  
    i2c_read_msg2133(&dbbus_rx_data[0], 2);  
    pr_tp("dbbus_rx_data[0]=0x%x", dbbus_rx_data[0]);  
    dbbus_tx_data[3] = dbbus_rx_data[0] & 0xFC;  //Clear Bit 1,0  
    i2c_write_msg2133(dbbus_tx_data, 4);  
    //WP overwrite  
    dbbus_tx_data[0] = 0x10;  
    dbbus_tx_data[1] = 0x1E;  
    dbbus_tx_data[2] = 0x0E;  
    dbbus_tx_data[3] = 0x02;  
    i2c_write_msg2133(dbbus_tx_data, 4);  
    //set pin high  
    dbbus_tx_data[0] = 0x10;  
    dbbus_tx_data[1] = 0x1E;  
    dbbus_tx_data[2] = 0x10;  
    dbbus_tx_data[3] = 0x08;  
    i2c_write_msg2133(dbbus_tx_data, 4);  
    dbbusDWIICIICNotUseBus();  
    dbbusDWIICNotStopMCU();  
    dbbusDWIICExitSerialDebugMode();  
    ///////////////////////////////////////  
    // Start to load firmware  
    ///////////////////////////////////////  
    drvISP_EntryIspMode();  
    pr_tp("chip erase+\n");  
    drvISP_BlockErase(0x00000);  
    pr_tp("chip erase-\n");  
    drvISP_ExitIspMode();  
    return size;  
}  
  
static DEVICE_ATTR(clear, 0777, firmware_clear_show, firmware_clear_store);  
  
/*test=================*/  
/*Add by Tracy.Lin for update touch panel firmware and get fw version*/  
  
static ssize_t firmware_version_show(struct device *dev,  
struct device_attribute *attr, char *buf)  
{  
    pr_ch("*** firmware_version_show fw_version = %s***\n", fw_version);  
    return sprintf(buf, "%s\n", fw_version);  
}  
  
static ssize_t firmware_version_store(struct device *dev,  
struct device_attribute *attr, const char *buf, size_t size)  
{  
    unsigned char dbbus_tx_data[3];  
    unsigned char dbbus_rx_data[4] ;  
    unsigned short major = 0, minor = 0;  
    dbbusDWIICEnterSerialDebugMode();  
    dbbusDWIICStopMCU();  
    dbbusDWIICIICUseBus();  
    dbbusDWIICIICReshape();  
    fw_version = kzalloc(sizeof(char), GFP_KERNEL);  
    pr_ch("\n");  
    //Get_Chip_Version();  
    dbbus_tx_data[0] = 0x53;  
    dbbus_tx_data[1] = 0x00;  
    dbbus_tx_data[2] = 0x74;  
    i2c_write(TOUCH_ADDR_MSG21XX, &dbbus_tx_data[0], 3);  
    i2c_read(TOUCH_ADDR_MSG21XX, &dbbus_rx_data[0], 4);  
    major = (dbbus_rx_data[1] << 8) + dbbus_rx_data[0];  
    minor = (dbbus_rx_data[3] << 8) + dbbus_rx_data[2];  
    pr_tp("***major = %d ***\n", major);  
    pr_tp("***minor = %d ***\n", minor);  
    sprintf(fw_version, "%03d%03d", major, minor);  
    pr_tp("***fw_version = %s ***\n", fw_version);  
    return size;  
}  
static DEVICE_ATTR(version, 0777, firmware_version_show, firmware_version_store);  
  
static ssize_t firmware_data_show(struct device *dev,  
struct device_attribute *attr, char *buf)  
{  
    return FwDataCnt;  
}  
  
static ssize_t firmware_data_store(struct device *dev,  
struct device_attribute *attr, const char *buf, size_t size)  
{  
    pr_ch("***FwDataCnt = %d ***\n", FwDataCnt);  
    if (download_firmware_buf == NULL) {  
        download_firmware_buf = kzalloc(DOWNLOAD_FIRMWARE_BUF_SIZE, GFP_KERNEL);  
        if (download_firmware_buf == NULL)  
            return NULL;  
    }  
    if(FwDataCnt<59)  
    {  
        memcpy(&download_firmware_buf[FwDataCnt*1024], buf, 1024);  
    }  
    FwDataCnt++;  
    return size;  
}  
static DEVICE_ATTR(data, 0777, firmware_data_show, firmware_data_store);  
#endif  
  
#ifdef TP_PROXIMITY_SENSOR  
static ssize_t show_proximity_sensor(struct device *dev, struct device_attribute *attr, char *buf)  
{  
    static char temp=2;  
    buf = ps_data_state;  
    if(temp!=*buf)  
    {  
        printk("proximity_sensor_show: buf=%d\n\n", *buf);  
        temp=*buf;  
    }    return 1;  
}  
  
static ssize_t store_proximity_sensor(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)  
{  
    U8 ps_store_data[4];  
  
    if(buf != NULL && size != 0)  
    {  
        if(DISABLE_CTP_PS == *buf)  
        {  
            printk("DISABLE_CTP_PS buf=%d,size=%d\n", *buf, size);  
            ps_store_data[0] = 0x52;  
            ps_store_data[1] = 0x00;  
            ps_store_data[2] = 0x62;  
            ps_store_data[3] = 0xa1;  
            i2c_write(TOUCH_ADDR_MSG21XX, &ps_store_data[0], 4);  
            msleep(2000);  
            printk("RESET_CTP_PS buf=%d\n", *buf);  
            mt65xx_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);  
            mt_set_gpio_mode(GPIO_CTP_MSG2133_EN_PIN, GPIO_CTP_MSG2133_EN_PIN_M_GPIO);  
            mt_set_gpio_dir(GPIO_CTP_MSG2133_EN_PIN, GPIO_DIR_OUT);  
            mt_set_gpio_out(GPIO_CTP_MSG2133_EN_PIN, GPIO_OUT_ZERO);  
            msleep(100);  
            mt_set_gpio_mode(GPIO_CTP_MSG2133_EN_PIN, GPIO_CTP_MSG2133_EN_PIN_M_GPIO);  
            mt_set_gpio_dir(GPIO_CTP_MSG2133_EN_PIN, GPIO_DIR_OUT);  
            mt_set_gpio_out(GPIO_CTP_MSG2133_EN_PIN, GPIO_OUT_ONE);  
            mdelay(500);  
            mt65xx_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);  
        }  
        else if(ENABLE_CTP_PS == *buf)  
        {  
            printk("ENABLE_CTP_PS buf=%d,size=%d\n", *buf, size);  
            ps_store_data[0] = 0x52;  
            ps_store_data[1] = 0x00;  
            ps_store_data[2] = 0x62;  
            ps_store_data[3] = 0xa0;  
            i2c_write(TOUCH_ADDR_MSG21XX, &ps_store_data[0], 4);  
        }  
    }  
  
    return size;  
}  
static DEVICE_ATTR(proximity_sensor, 0777, show_proximity_sensor, store_proximity_sensor);  
#endif  
  
static struct i2c_driver tpd_i2c_driver =  
{  
    .driver = {  
        .name = "msg2133",  
        .owner = THIS_MODULE,  
    },  
    .probe = tpd_probe,  
    .remove = __devexit_p(tpd_remove),  
    .id_table = tpd_id,  
    .detect = tpd_detect,  
    .address_data = &addr_data,  
};  
  
  
static void tpd_down(int x, int y, int p)  
{  
    input_report_abs(tpd->dev, ABS_PRESSURE, p);  
    input_report_key(tpd->dev, BTN_TOUCH, 1);  
    input_report_abs(tpd->dev, ABS_MT_TOUCH_MAJOR, p);  
    input_report_abs(tpd->dev, ABS_MT_POSITION_X, x);  
    input_report_abs(tpd->dev, ABS_MT_POSITION_Y, y);  
    printk("[MSG2133]######tpd_down[%4d %4d %4d] ", x, y, p);  
    input_mt_sync(tpd->dev);  
    TPD_DOWN_DEBUG_TRACK(x,y);  
}  
  
static  int tpd_up(int x, int y)  
{  
    #ifdef TPD_HAVE_BUTTON  
    if (y > TPD_BUTTON_HEIGHT)  
    {  
        tpd_button(x, y,0);  
        return;  
    }  
    #endif  
    input_mt_sync(tpd->dev);  
    input_sync(tpd->dev);  
    TPD_DOWN_DEBUG_TRACK(x, y);  
    return 1;  
}  
unsigned char tpd_check_sum(unsigned char *pval)  
{  
    int i, sum = 0;  
  
    for(i = 0; i < 7; i++)  
    {  
        sum += pval[i];  
    }  
  
    return (unsigned char)((-sum) & 0xFF);  
}  
  
static bool msg2133_i2c_read(char *pbt_buf, int dw_lenth)  
{  
    int ret;  
    i2c_client->timing = 100;  
    i2c_client->addr|=I2C_ENEXT_FLAG;  
    ret = i2c_master_recv(i2c_client, pbt_buf, dw_lenth);  
  
    if(ret <= 0)  
    {  
        pr_tp("msg_i2c_read_interface error\n");  
        return false;  
    }  
  
    return true;  
}  
  
static int i2c_master_recv_ext(struct i2c_client *client, char *buf ,int count)  
{  
    u32 phyAddr = 0;   
    u8  buf_dma[8] = {0};  
    u32 old_addr = 0;   
    int ret = 0;   
    int retry = 3;   
    int i = 0;   
   // u8  *buf_test ;  
//  buf_test = &buf_dma[0];  
  
    old_addr = client->addr;   
    client->addr |= I2C_ENEXT_FLAG ;  
      
    printk("[MSG2133][i2c_master_recv_ext] client->addr = %x\n", client->addr);   
  
    do {  
        ret = i2c_master_recv(client, buf_dma, count);     
        retry --;   
        if (ret != count) {  
            printk("[MSG2133][i2c_master_recv_ext] Error sent I2C ret = %d\n", ret);   
        }  
    }while ((ret != count) && (retry > 0));   
      
    memcpy(buf, buf_dma, count);   
      
    client->addr = old_addr;   
  
    return ret;   
}  
  
static int tpd_touchinfo(struct TouchInfoT *cinfo, struct TouchInfoT *pinfo)  
{  
    u32 retval;  
    u8 key;  
    u8 touchData = 0;  
      
    //pinfo->count = cinfo->count;  
    memcpy(pinfo, cinfo, sizeof(struct TouchInfoT));  
      
    //add for sure addr correct  
    //i2c_client->addr = 0x4c;  
    retval = i2c_master_recv_ext(i2c_client, (u8 *)&TpdTouchData, sizeof(TpdTouchData));  
  
    if(TpdTouchData.packet_id != 0x52 )  
       {  
        return 0;  
       }  
    /*touch*/  
    if(TpdTouchData.packet_id == 0x52)  
    {  
      
        if(TpdTouchData.x_h_y_h == 0xFF   
            && TpdTouchData.x_l == 0xFF   
            && TpdTouchData.y_l == 0xFF   
            && TpdTouchData.disx_h_disy_h == 0xFF   
          )  
        {  
#ifdef TPD_HAVE_BUTTON  
         {  
            U8 *p = &TpdTouchData;  
            cinfo->key_value = 0;  
            cinfo->key_value = *(p+5);             
            printk("+++++++zym+++++++TPD_HAVE_BUTTON:(%d)\n",cinfo->key_value);  
            {  
                //tpd_button_msg213x(cinfo->key_value);  
                //tpd_button(0,0,cinfo->key_value);  
                if(cinfo->key_value == 1)  
                {  
#ifdef HQ_A25_NANBO_CTP_MSG2133_KEY  
                             touchData = KEY_BACK;    
#else    
                             touchData = KEY_MENU;  
#endif  
                }  
                else if(cinfo->key_value == 2)  
                {  
#ifdef HQ_CTP_MSG21XX_REVERT  
                    touchData = KEY_BACK;  
#elif defined(HQ_A25_NANBO_CTP_MSG2133_KEY)  
                                        touchData = KEY_MENU;  
#elif defined(HQ_A25P_MUDONG_CTP_MSG2133_KEY)  
                                         touchData = KEY_BACK;  
#else  
                    touchData = KEY_HOME;  
#endif  
                }  
                else if(cinfo->key_value == 4)  
                {  
                    touchData = KEY_BACK;  
                }  
                else if(cinfo->key_value == 8)  
                {  
                    touchData = KEY_SEARCH;  
                }  
                else  
                {  
                    touchData = 0;  
                }  
                TPD_LOGV("[MSG2133]+++++++zym+++++++:(%d)\n",touchData);  
                if(touchData)  
                    input_report_key(tpd->dev,touchData,1);  
                else  
                {  
                    input_report_key(tpd->dev,KEY_MENU,0);  
                    input_report_key(tpd->dev,KEY_HOME,0);  
                    input_report_key(tpd->dev,KEY_BACK,0);  
                    input_report_key(tpd->dev,KEY_SEARCH,0);  
                }  
            }  
         }  
#endif  
            cinfo->count = 0;  
        }  
        else if(TpdTouchData.disx_h_disy_h == 0  
            && TpdTouchData.disx_l == 0   
            && TpdTouchData.disy_l == 0)  
            cinfo->count = 1;  
        else  
            cinfo->count = 2;  
          
        TPD_LOGV("[MSG2133]cinfo: count=%d\n",cinfo->count);  
        if(cinfo->count > 0)  
        {  
            int tmp_x,tmp_y;  
            /*point1*/  
            cinfo->x1 = (((TpdTouchData.x_h_y_h&0xF0)<<4) | (TpdTouchData.x_l));  
            cinfo->y1 = (((TpdTouchData.x_h_y_h&0x0F)<<8) | (TpdTouchData.y_l));  
            TPD_LOGV("[MSG2133]+++zym+++(%3d,%3d)\n",cinfo->x1,cinfo->y1);  
  
            if(cinfo->count >1)  
            {     
                /*point2*/  
                short disx,disy;  
      
                disx = (((TpdTouchData.disx_h_disy_h&0xF0)<<4) | (TpdTouchData.disx_l));  
                disy = (((TpdTouchData.disx_h_disy_h&0x0F)<<8) | (TpdTouchData.disy_l));  
                disy = (disy<<4);  
                disy = disy/16;  
                if(disx >= 2048)  
                    disx -= 4096;  
                if(disy >= 2048)  
                    disy -= 4096;  
                cinfo->x2 = cinfo->x1 + disx;  
                cinfo->y2 = cinfo->y1 + disy;               
  
                tmp_x = cinfo->x2;  
                tmp_y = cinfo->y2;  
                cinfo->y2 = tmp_x * (TPD_RES_Y - 1)/ 2047;  
                cinfo->x2 = tmp_y * (TPD_RES_X - 1) / 2047;        
            }  
            tmp_x = cinfo->x1;  
            tmp_y = cinfo->y1;  
            cinfo->y1 = tmp_x * (TPD_RES_Y - 1) / 2047;  
            cinfo->x1 = tmp_y * (TPD_RES_X - 1) / 2047;  
              
            //add by zym 2012-04-16  
            #ifdef HQ_CTP_MSG21XX_REVERT  
            #else  
            cinfo->x1 = TPD_RES_X -1 - cinfo->x1;  
            cinfo->x2 = TPD_RES_X - 1 -cinfo->x2;  
            #endif  
            TPD_LOGV("[MSG2133]++++++++zym+++++++++p1: (%3d,%3d)(%3d,%3d)\n",cinfo->x1,cinfo->y1,TPD_RES_X,TPD_RES_Y);  
            cinfo->pressure = 1;  
            TPD_LOGV("[MSG2133]pressure: %d\n",cinfo->pressure);  
        }  
    }  
    else  
    {  
        cinfo->count = 0;  
    }  
  
    /*ergate-012 start*/  
    /*ergate-012 end*/  
  
    return 1;  
}  
  
  
  
static int touch_event_handler(void *unused)  
{  
  
    int pending = 0;  
    struct TouchInfoT cinfo, pinfo;  
    struct sched_param param = { .sched_priority = RTPM_PRIO_TPD };  
    sched_setscheduler(current, SCHED_RR, ¶m);  
  
    memset(&cinfo, 0, sizeof(struct TouchInfoT));  
    memset(&pinfo, 0, sizeof(struct TouchInfoT));  
    do  
    {  
      
        set_current_state(TASK_INTERRUPTIBLE);   
        if(!kthread_should_stop())  
        {  
            TPD_DEBUG_CHECK_NO_RESPONSE;  
            do  
            {  
                if (pending)   
                    wait_event_interruptible_timeout(waiter, tpd_flag != 0, HZ/10);  
                else   
                    wait_event_interruptible_timeout(waiter,tpd_flag != 0, HZ*2);  
                   
            }while(0);  
               
            if (tpd_flag == 0 && !pending)   
                continue; // if timeout for no touch, then re-wait.  
               
            if (tpd_flag != 0 && pending > 0)      
                pending = 0;  
               
            tpd_flag = 0;  
            TPD_DEBUG_SET_TIME;   
        }  
        set_current_state(TASK_RUNNING);  
  
        if (tpd_touchinfo(&cinfo, &pinfo))  
        {  
            if(cinfo.count >0)  
            {  
                tpd_down(cinfo.x1, cinfo.y1, cinfo.pressure);  
                if(cinfo.count>1)  
                {                 
                    tpd_down(cinfo.x2, cinfo.y2, cinfo.pressure);  
                }  
                input_sync(tpd->dev);          
                TPD_LOGV("[MSG2133]press  --->\n");  
            }  
            else if(cinfo.count==0 && pinfo.count!=0)  
            {         
       
                input_mt_sync(tpd->dev);  
                input_sync(tpd->dev);  
          
                TPD_LOGV("[MSG2133]release --->\n");   
            }  
        }            
    }while(!kthread_should_stop());  
  
    return 0;  
  
}  
  
static int tpd_detect(struct i2c_client *client, int kind, struct i2c_board_info *info)  
{  
    strcpy(info->type, "msg2133");  
    return 0;  
}  
  
static void tpd_eint_interrupt_handler(void)  
{  
    tpd_flag = 1;  
    wake_up_interruptible(&waiter);  
}  
  
static int __devinit tpd_probe(struct i2c_client *client, const struct i2c_device_id *id)  
{  
    int retval = TPD_OK;  
    client->timing = 100;  
    i2c_client = client;  
    msleep(10);  
    hwPowerOn(MT65XX_POWER_LDO_VGP2, VOL_2800, "TP");  
    mt_set_gpio_mode(GPIO_CTP_MSG2133_EN_PIN, GPIO_CTP_MSG2133_EN_PIN_M_GPIO);  
    mt_set_gpio_dir(GPIO_CTP_MSG2133_EN_PIN, GPIO_DIR_OUT);  
    mt_set_gpio_out(GPIO_CTP_MSG2133_EN_PIN, GPIO_OUT_ONE);  
  
    msleep(100);  
    mt_set_gpio_mode(GPIO_CTP_MSG2133_EINT_PIN, GPIO_CTP_MSG2133_EINT_PIN_M_GPIO);  
    mt_set_gpio_dir(GPIO_CTP_MSG2133_EINT_PIN, GPIO_DIR_IN);  
    mt_set_gpio_pull_enable(GPIO_CTP_MSG2133_EINT_PIN, GPIO_PULL_ENABLE);  
    mt_set_gpio_pull_select(GPIO_CTP_MSG2133_EINT_PIN, GPIO_PULL_UP);  
  
  
    mt65xx_eint_set_sens(CUST_EINT_TOUCH_PANEL_NUM, CUST_EINT_TOUCH_PANEL_SENSITIVE);  
    mt65xx_eint_set_hw_debounce(CUST_EINT_TOUCH_PANEL_NUM, CUST_EINT_TOUCH_PANEL_DEBOUNCE_CN);  
    mt65xx_eint_registration(CUST_EINT_TOUCH_PANEL_NUM, CUST_EINT_TOUCH_PANEL_DEBOUNCE_EN, CUST_EINT_TOUCH_PANEL_POLARITY, tpd_eint_interrupt_handler, 1);  
    mt65xx_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);  
    msleep(100);  
    tpd_load_status = 1;  
    mthread = kthread_run(touch_event_handler, 0, "msg2133");  
  
    if(IS_ERR(mthread))  
    {  
        retval = PTR_ERR(mthread);  
        pr_tp(TPD_DEVICE " failed to create kernel thread: %d\n", retval);  
    }  
#ifdef TP_FIRMWARE_UPDATE  
#if ENABLE_DMA  
    gpDMABuf_va = (u8 *)dma_alloc_coherent(NULL, 4096, &gpDMABuf_pa, GFP_KERNEL);  
  
    if(!gpDMABuf_va)  
    {  
        printk("[MATV][Error] Allocate DMA I2C Buffer failed!\n");  
    }  
  
#endif  
    firmware_class = class_create(THIS_MODULE, "ms-touchscreen-msg20xx");  
  
    if(IS_ERR(firmware_class))  
    {  
        pr_err("Failed to create class(firmware)!\n");  
    }  
  
    firmware_cmd_dev = device_create(firmware_class,NULL, 0, NULL, "device");  
  
    if(IS_ERR(firmware_cmd_dev))  
    {  
        pr_err("Failed to create device(firmware_cmd_dev)!\n");  
    }  
  
    // version  
    if(device_create_file(firmware_cmd_dev, &dev_attr_version) < 0)  
    {  
        pr_err("Failed to create device file(%s)!\n", dev_attr_version.attr.name);  
    }  
  
    // update  
    if(device_create_file(firmware_cmd_dev, &dev_attr_update) < 0)  
    {  
        pr_err("Failed to create device file(%s)!\n", dev_attr_update.attr.name);  
    }  
  
    // data  
    if(device_create_file(firmware_cmd_dev, &dev_attr_data) < 0)  
    {  
        pr_err("Failed to create device file(%s)!\n", dev_attr_data.attr.name);  
    }  
  
    if(device_create_file(firmware_cmd_dev, &dev_attr_clear) < 0)  
    {  
        pr_err("Failed to create device file(%s)!\n", dev_attr_clear.attr.name);  
    }  
  
#endif  
#ifdef TP_PROXIMITY_SENSOR  
  
    if(device_create_file(firmware_cmd_dev, &dev_attr_proximity_sensor) < 0) // /sys/class/mtk-tpd/device/proximity_sensor  
    {  
        pr_err("Failed to create device file(%s)!\n", dev_attr_proximity_sensor.attr.name);  
    }  
#endif  
    printk("Touch Panel Device Probe %s\n", (retval < TPD_OK) ? "FAIL" : "PASS");  
    return 0;  
}  
  
static int __devexit tpd_remove(struct i2c_client *client)  
  
{  
    printk("TPD removed\n");  
#ifdef TP_FIRMWARE_UPDATE  
#if ENABLE_DMA  
  
    if(gpDMABuf_va)  
    {  
        dma_free_coherent(NULL, 4096, gpDMABuf_va, gpDMABuf_pa);  
        gpDMABuf_va = NULL;  
        gpDMABuf_pa = NULL;  
    }  
  
#endif  
#endif  
    return 0;  
}  
  
  
static int tpd_local_init(void)  
{  
    printk(" MSG2133 I2C Touchscreen Driver (Built %s @ %s)\n", __DATE__, __TIME__);  
  
    if(i2c_add_driver(&tpd_i2c_driver) != 0)  
    {  
        printk("unable to add i2c driver.\n");  
        return -1;  
    }  
  
#ifdef TPD_HAVE_BUTTON  
    tpd_button_setting(TPD_KEY_COUNT, tpd_keys_local, tpd_keys_dim_local);// initialize tpd button data  
#endif  
    pr_tp("end %s, %d\n", __FUNCTION__, __LINE__);  
    tpd_type_cap = 1;  
    return 0;  
}  
  
static int tpd_resume(struct i2c_client *client)  
{  
    int retval = TPD_OK;  
    printk("TPD wake up\n");  
  
     hwPowerOn(MT65XX_POWER_LDO_VGP2, VOL_2800, "TP");  
     msleep(10);  
    mt_set_gpio_mode(GPIO_CTP_MSG2133_EN_PIN, GPIO_CTP_MSG2133_EN_PIN_M_GPIO);  
    mt_set_gpio_dir(GPIO_CTP_MSG2133_EN_PIN, GPIO_DIR_OUT);  
    mt_set_gpio_out(GPIO_CTP_MSG2133_EN_PIN, GPIO_OUT_ONE);  
  
    msleep(200);  
    mt65xx_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);  
    mt65xx_eint_registration(CUST_EINT_TOUCH_PANEL_NUM, CUST_EINT_TOUCH_PANEL_DEBOUNCE_EN, CUST_EINT_TOUCH_PANEL_POLARITY, tpd_eint_interrupt_handler, 1);  
  
    return retval;  
}  
  
static int tpd_suspend(struct i2c_client *client, pm_message_t message)  
{  
    int retval = TPD_OK;  
    static char data = 0x3;  
    printk("TPD enter sleep\n");  
  
    {  
        mt65xx_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);  
        mt65xx_eint_registration(CUST_EINT_TOUCH_PANEL_NUM, CUST_EINT_TOUCH_PANEL_DEBOUNCE_EN, CUST_EINT_TOUCH_PANEL_POLARITY, NULL, 1);  
  
  
        mt_set_gpio_mode(GPIO_CTP_MSG2133_EN_PIN, GPIO_CTP_MSG2133_EN_PIN_M_GPIO);  
        mt_set_gpio_dir(GPIO_CTP_MSG2133_EN_PIN, GPIO_DIR_OUT);  
        mt_set_gpio_out(GPIO_CTP_MSG2133_EN_PIN, GPIO_OUT_ZERO);  
      
        hwPowerDown(MT65XX_POWER_LDO_VGP2, "TP");  
      
    }  
    return retval;  
}  
  
int tpd_power_switch( int flag )  
{  
    return 0;  
}  
  
EXPORT_SYMBOL( tpd_power_switch );  
  
static struct tpd_driver_t tpd_device_driver =  
{  
    .tpd_device_name = "msg2133",  
    .tpd_local_init = tpd_local_init,  
    .suspend = tpd_suspend,  
    .resume = tpd_resume,  
#ifdef TPD_HAVE_BUTTON  
    .tpd_have_button = 1,  
#else  
    .tpd_have_button = 0,  
#endif  
};  
/* called when loaded into kernel */  
static int __init tpd_driver_init(void)  
{  
    printk("MediaTek MSG2133 touch panel driver init\n");  
    if(tpd_driver_add(&tpd_device_driver) < 0)  
    {  
        printk("add MSG2133 driver failed\n");  
    }  
  
    return 0;  
}  
  
/* should never be called */  
static void __exit tpd_driver_exit(void)  
{  
    printk("MediaTek MSG2133 touch panel driver exit\n");  
    //input_unregister_device(tpd->dev);  
    tpd_driver_remove(&tpd_device_driver);  
}  
  
module_init(tpd_driver_init);  
module_exit(tpd_driver_exit); 
