#include "stdio.h"
#include "string.h"

#include "xparameters.h"
#include "xspips.h"
#include "xgpiops.h"
#include "xuartps.h"
#include "xscugic.h"

#include "xil_printf.h"
#include "sleep.h"

// 设备ID
#define GPIOPS_ID XPAR_XGPIOPS_0_DEVICE_ID
#define SPIPS_ID XPAR_XSPIPS_0_DEVICE_ID
#define UART_DEVICE_ID XPAR_PS7_UART_0_DEVICE_ID
#define INTC_DEVICE_ID XPAR_SCUGIC_SINGLE_DEVICE_ID
#define UART_INT_IRQ_ID XPAR_XUARTPS_0_INTR

// GPIOPS的引脚号
#define RUN_LED 7
#define SYS_RES_LED 8

#define FRAME_REQ 54
#define SYS_RES_N 55
#define SENSOR_SWITCH 56

#define F 48.0
#define OFFSET 1000.0 * F / 40.0
#define SLOPE 12.0 / F

// 设置控制信号
#define set_frame_req(x) XGpioPs_WritePin(&gpiops_inst, FRAME_REQ, x)
#define set_sys_res_n(x) XGpioPs_WritePin(&gpiops_inst, SYS_RES_N, x)
#define set_run_led(x) XGpioPs_WritePin(&gpiops_inst, RUN_LED, x)
#define set_res_led(x) XGpioPs_WritePin(&gpiops_inst, SYS_RES_LED, x)

XSpiPs Spi0;
XGpioPs gpiops_inst;
XScuGic Intc;
XUartPs Uart_Ps;

int sensor_switch = 0;

int spi0_init();
int gpio0_init();

void spi_upload();
void start_up_sequence();
void reset_sequence();
void end_sequence();

int uart_init(XUartPs *uart_ps);
void uart_intr_handler(void *call_back_ref);
int uart_intr_init(XScuGic *intc, XUartPs *uart_ps);

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

    Status = uart_init(&Uart_Ps); // 串口初始�?????
    if (Status == XST_FAILURE)
    {
        xil_printf("Uart Initial Failed\r\n");
        return XST_FAILURE;
    }
    uart_intr_init(&Intc, &Uart_Ps); // 串口中断初始�?????

    while (1)
    {

        XSpiPs_PolledTransfer(&Spi0, write_temperature_reg, read_temperature_reg, 4);

        double tem = (read_temperature_reg[3] * 256 + read_temperature_reg[1] - OFFSET) * SLOPE;

        printf("REG-127 = %#x\t", read_temperature_reg[3]);
        printf("REG-126 = %#x\n", read_temperature_reg[1]);
        printf("Temperature = %.2lf��", tem);

        printf("\n\n");
        memset(read_temperature_reg, 0x00, 4);
        sleep(5);
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

    // Status = XSpiPs_SetDelays(&Spi0, 0x00, 0x00, 0x00, 0x00);
    // if (Status != XST_SUCCESS)
    // {
    //     xil_printf("%s XSpiPs_SetDelays fail\n", __func__);
    //     return XST_FAILURE;
    // }
    XSpiPs_Enable(&Spi0);
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


    XGpioPs_SetDirectionPin(&gpiops_inst, RUN_LED, 1);
    XGpioPs_SetDirectionPin(&gpiops_inst, SYS_RES_LED, 1);
    XGpioPs_SetDirectionPin(&gpiops_inst, FRAME_REQ, 1);
    XGpioPs_SetDirectionPin(&gpiops_inst, SYS_RES_N, 1);
    XGpioPs_SetDirectionPin(&gpiops_inst, SENSOR_SWITCH, 1);

    XGpioPs_SetOutputEnablePin(&gpiops_inst, RUN_LED, 1);
    XGpioPs_SetOutputEnablePin(&gpiops_inst, SYS_RES_LED, 1);
    XGpioPs_SetOutputEnablePin(&gpiops_inst, FRAME_REQ, 1);
    XGpioPs_SetOutputEnablePin(&gpiops_inst, SYS_RES_N, 1);
    XGpioPs_SetOutputEnablePin(&gpiops_inst, SENSOR_SWITCH, 1);

    XGpioPs_WritePin(&gpiops_inst, RUN_LED, 0);
    XGpioPs_WritePin(&gpiops_inst, SYS_RES_LED, 0);
    XGpioPs_WritePin(&gpiops_inst, FRAME_REQ, 0);
    XGpioPs_WritePin(&gpiops_inst, SYS_RES_N, 0);
    XGpioPs_WritePin(&gpiops_inst, SENSOR_SWITCH, 0);

    return XST_SUCCESS;
}

void spi_upload()
{
    printf("\nspi_upload\n\n");
    XSpiPs_PolledTransfer(&Spi0, write_frame_num_reg, read_frame_num_reg, 4);
}

void start_up_sequence()
{
    printf("\nstart_up_sequence\n\n");
    set_sys_res_n(1);
    usleep(1);
    spi_upload();
    usleep(5000);
    set_frame_req(1);
    set_run_led(1);
}

void reset_sequence()
{
    printf("\nreset_sequence\n\n");
    set_res_led(1);
    set_frame_req(0);
    set_sys_res_n(0);
    set_sys_res_n(1);

    usleep(1);
    spi_upload();
    usleep(5000);
    set_frame_req(1);
    set_res_led(0);
}

void end_sequence()
{
    printf("\nend_sequence\n\n");
    set_frame_req(0);
    set_sys_res_n(0);
    set_run_led(0);
}

// UART初始化函�?????
int uart_init(XUartPs *uart_ps)
{
    int status;
    XUartPs_Config *uart_cfg;
    XUartPsFormat format = {115200, XUARTPS_FORMAT_8_BITS, XUARTPS_FORMAT_NO_PARITY, XUARTPS_FORMAT_1_STOP_BIT};

    uart_cfg = XUartPs_LookupConfig(UART_DEVICE_ID);
    if (NULL == uart_cfg)
        return XST_FAILURE;
    status = XUartPs_CfgInitialize(uart_ps, uart_cfg, uart_cfg->BaseAddress);
    if (status != XST_SUCCESS)
        return XST_FAILURE;

    // UART设备自检
    status = XUartPs_SelfTest(uart_ps);
    if (status != XST_SUCCESS)
        return XST_FAILURE;

    XUartPs_EnableUart(uart_ps);
    // 设置工作模式:正常模式
    XUartPs_SetOperMode(uart_ps, XUARTPS_OPER_MODE_NORMAL);
    XUartPs_SetDataFormat(uart_ps, &format);
    // 设置波特�?????:115200
    // XUartPs_SetBaudRate(uart_ps, 115200);
    // 设置RxFIFO的中断触发等�?????
    XUartPs_SetFifoThreshold(uart_ps, 5);
    XUartPs_SetInterruptMask(uart_ps, XUARTPS_IXR_RXOVR);

    return XST_SUCCESS;
}

// UART中断处理函数
void uart_intr_handler(void *call_back_ref)
{
    XUartPs *uart_instance_ptr = (XUartPs *)call_back_ref;
    int index = 0;
    char rec_data[10] = "";
    u32 isr_status; // 中断状�?�标�?????

    // 读取中断ID寄存器，判断触发的是哪种中断
    isr_status = XUartPs_ReadReg(uart_instance_ptr->Config.BaseAddress, XUARTPS_IMR_OFFSET);
    isr_status &= XUartPs_ReadReg(uart_instance_ptr->Config.BaseAddress, XUARTPS_ISR_OFFSET);

    // 判断中断标志位RxFIFO是否触发
    if (isr_status & (u32)XUARTPS_IXR_RXOVR)
    {
        isr_status = XUartPs_ReadReg(uart_instance_ptr->Config.BaseAddress, XUARTPS_SR_OFFSET);
        while ((isr_status & XUARTPS_SR_RXEMPTY) == 0)
        {
            rec_data[index] = XUartPs_RecvByte(XPAR_PS7_UART_0_BASEADDR);
            index++;
            isr_status = XUartPs_ReadReg(uart_instance_ptr->Config.BaseAddress, XUARTPS_SR_OFFSET);
        }
        rec_data[index] = '\0';
        if (strcmp(rec_data, "start") == 0)
        {
            start_up_sequence();
        }
        else if (strcmp(rec_data, "reset") == 0)
        {
            reset_sequence();
        }
        else if (strcmp(rec_data, "endll") == 0)
        {
            end_sequence();
        }
        else
        {
            printf("command fail\n");
        }
        // 清除中断标志
        XUartPs_WriteReg(uart_instance_ptr->Config.BaseAddress, XUARTPS_ISR_OFFSET, XUARTPS_IXR_RXOVR);
    }
    else
    {
        printf("rxovr fail\n");
    }
}

// 串口中断初始�?????
int uart_intr_init(XScuGic *intc, XUartPs *uart_ps)
{
    int status;
    // 初始化中断控制器
    XScuGic_Config *intc_cfg;
    intc_cfg = XScuGic_LookupConfig(INTC_DEVICE_ID);
    if (NULL == intc_cfg)
        return XST_FAILURE;
    status = XScuGic_CfgInitialize(intc, intc_cfg, intc_cfg->CpuBaseAddress);
    if (status != XST_SUCCESS)
        return XST_FAILURE;

    // 设置并打�?????中断异常处理功能
    Xil_ExceptionInit();
    Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
                                 (Xil_ExceptionHandler)XScuGic_InterruptHandler,
                                 (void *)intc);
    Xil_ExceptionEnable();

    // 为中断设置中断处理函�?????
    XScuGic_Connect(intc, UART_INT_IRQ_ID,
                    (Xil_ExceptionHandler)uart_intr_handler, (void *)uart_ps);
    // 设置UART的中断触发方�?????
    // 使能GIC中的串口中断
    XScuGic_Enable(intc, UART_INT_IRQ_ID);
    return XST_SUCCESS;
}
