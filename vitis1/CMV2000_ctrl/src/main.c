#include <stdio.h>
#include "xparameters.h"

#include "xspips.h"
#include "xgpiops.h"

#include "xil_printf.h"
#include "sleep.h"

#define GPIOPS_ID XPAR_XGPIOPS_0_DEVICE_ID
#define SPIPS_ID XPAR_XSPIPS_0_DEVICE_ID

#define SENSOR_LED 7
#define SYS_RES_LED 8

#define SENSOR_KEY 12
#define SYS_RES_KEY 11

#define FRAME_REQ 54
#define SYS_RES_N 55
#define SENSOR_SWITCH 56

#define F 48.0
#define OFFSET 1000.0 * F / 40.0
#define SLOPE 12.0 / F

XSpiPs Spi0;
XGpioPs gpiops_inst;

int sensor_switch = 0;

int spi0_init();
int gpio0_init();

void frame_req_pulse();
void spi_upload();
void start_up_sequence();
void reset_sequence();

u8 read_temperature_reg[4] = {0, 0, 0, 0};
u8 write_temperature_reg[4] = {0x7E, 0xFF, 0x7F, 0xFF};

u8 read_frame_num_reg[4] = {0, 0, 0, 0};
u8 write_frame_num_reg[4] = {0xC7, 0x01, 0xC6, 0x52};

int main()
{
    int Status;

    Status = gpio0_init();
    if (Status != XST_SUCCESS)
    {
        xil_printf("GPIO  Failed\r\n");
        return XST_FAILURE;
    }

    Status = spi0_init();
    if (Status != XST_SUCCESS)
    {
        xil_printf("SPI Selftest Example Failed\r\n");
        return XST_FAILURE;
    }

    start_up_sequence();

    while (1)
    {

        if (XGpioPs_ReadPin(&gpiops_inst, SYS_RES_KEY) == 0)
        {
            usleep(2000);
            if (XGpioPs_ReadPin(&gpiops_inst, SYS_RES_KEY) == 0)
            {
                XGpioPs_WritePin(&gpiops_inst, SYS_RES_LED, 1);
                reset_sequence();
                XGpioPs_WritePin(&gpiops_inst, SYS_RES_LED, 0);
            }
        }

        if (XGpioPs_ReadPin(&gpiops_inst, SENSOR_KEY) == 0)
        {
            usleep(2000);
            if (XGpioPs_ReadPin(&gpiops_inst, SENSOR_KEY) == 0)
            {
                sensor_switch = ~sensor_switch;
                XGpioPs_WritePin(&gpiops_inst, SENSOR_SWITCH, sensor_switch);
                XGpioPs_WritePin(&gpiops_inst, SENSOR_LED, sensor_switch);
            }
        }

        frame_req_pulse();

        XSpiPs_PolledTransfer(&Spi0, write_temperature_reg, read_temperature_reg, 4);

        double tem = (read_temperature_reg[3] * 256 + read_temperature_reg[1] - OFFSET) * SLOPE;

        printf("REG-127 = %#x\t", read_temperature_reg[3]);
        printf("REG-126 = %#x\n", read_temperature_reg[1]);
        printf("Temperature = %.2lf��", tem);

        printf("\n\n");
        memset(read_temperature_reg, 0x00, 128);
        sleep(1);
    }

    return XST_SUCCESS;
}

int spi0_init()
{
    int Status;
    XSpiPs_Config *SpiConfig;

    /*
     * Initialize the SPI device.
     */
    SpiConfig = XSpiPs_LookupConfig(SPIPS_ID);
    if (NULL == SpiConfig)
    {
        return XST_FAILURE;
    }

    Status = XSpiPs_CfgInitialize(&Spi0, SpiConfig, SpiConfig->BaseAddress);
    if (Status != XST_SUCCESS)
    {
        return XST_FAILURE;
    }

    /*
     * Perform a self-test to check hardware build.
     */
    Status = XSpiPs_SelfTest(&Spi0);
    if (Status != XST_SUCCESS)
    {
        return XST_FAILURE;
    }
    xil_printf("%s self test succ\r\n", __func__);

    Status = XSpiPs_SetOptions(&Spi0, XSPIPS_MASTER_OPTION);
    if (Status != XST_SUCCESS)
    {
        xil_printf("%s SetOptions fail\n", __func__);
        return XST_FAILURE;
    }

    Status = XSpiPs_SetClkPrescaler(&Spi0, XSPIPS_CLK_PRESCALE_8);
    if (Status != XST_SUCCESS)
    {
        xil_printf("%s XSpiPs_SetClkPrescaler fail\n", __func__);
        return XST_FAILURE;
    }

    Status = XSpiPs_SetDelays(&Spi0, 0x00, 0x00, 0x00, 0x00);
    if (Status != XST_SUCCESS)
    {
        xil_printf("%s XSpiPs_SetDelays fail\n", __func__);
        return XST_FAILURE;
    }

    xil_printf("spi 0 config finish\n");
    return XST_SUCCESS;
}

int gpio0_init()
{
    int Status;

    XGpioPs_Config *gpiops_cfg_ptr;

    gpiops_cfg_ptr = XGpioPs_LookupConfig(GPIOPS_ID);
    if (NULL == gpiops_cfg_ptr)
    {
        return XST_FAILURE;
    }

    Status = XGpioPs_CfgInitialize(&gpiops_inst, gpiops_cfg_ptr,
                                   gpiops_cfg_ptr->BaseAddr);
    if (Status != XST_SUCCESS)
    {
        return XST_FAILURE;
    }

    XGpioPs_SetDirectionPin(&gpiops_inst, SENSOR_KEY, 0);
    XGpioPs_SetDirectionPin(&gpiops_inst, SYS_RES_KEY, 0);

    XGpioPs_SetDirectionPin(&gpiops_inst, SENSOR_LED, 1);
    XGpioPs_SetDirectionPin(&gpiops_inst, SYS_RES_LED, 1);
    XGpioPs_SetDirectionPin(&gpiops_inst, FRAME_REQ, 1);
    XGpioPs_SetDirectionPin(&gpiops_inst, SYS_RES_N, 1);
    XGpioPs_SetDirectionPin(&gpiops_inst, SENSOR_SWITCH, 1);

    XGpioPs_SetOutputEnablePin(&gpiops_inst, SENSOR_LED, 1);
    XGpioPs_SetOutputEnablePin(&gpiops_inst, SYS_RES_LED, 1);
    XGpioPs_SetOutputEnablePin(&gpiops_inst, FRAME_REQ, 1);
    XGpioPs_SetOutputEnablePin(&gpiops_inst, SYS_RES_N, 1);
    XGpioPs_SetOutputEnablePin(&gpiops_inst, SENSOR_SWITCH, 1);

    XGpioPs_WritePin(&gpiops_inst, SENSOR_LED, 0);
    XGpioPs_WritePin(&gpiops_inst, SYS_RES_LED, 0);
    XGpioPs_WritePin(&gpiops_inst, FRAME_REQ, 0);
    XGpioPs_WritePin(&gpiops_inst, SYS_RES_N, 0);
    XGpioPs_WritePin(&gpiops_inst, SENSOR_SWITCH, 0);

    return XST_SUCCESS;
}

void frame_req_pulse()
{
    printf("\nframe_req_pulse\n\n");
    XGpioPs_WritePin(&gpiops_inst, FRAME_REQ, 1);
    XGpioPs_WritePin(&gpiops_inst, FRAME_REQ, 0);
}

void spi_upload()
{
    printf("\nspi_upload\n\n");
    XSpiPs_PolledTransfer(&Spi0, write_frame_num_reg, read_frame_num_reg, 4);
}

void start_up_sequence()
{
    printf("\nstart_up_sequence\n\n");
    XGpioPs_WritePin(&gpiops_inst, SYS_RES_N, 1);
    usleep(1);
    spi_upload();
    usleep(7000);
    frame_req_pulse();
}

void reset_sequence()
{
    printf("\nreset_sequence\n\n");
    XGpioPs_WritePin(&gpiops_inst, SYS_RES_N, 0);
    XGpioPs_WritePin(&gpiops_inst, SYS_RES_N, 1);

    usleep(1);
    spi_upload();
    usleep(7000);
    frame_req_pulse();
}
