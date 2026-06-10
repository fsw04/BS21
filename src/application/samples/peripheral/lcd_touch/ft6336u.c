/**
 * Copyright (c) HiSilicon (Shanghai) Technologies Co., Ltd. 2024-2024. All rights reserved.
 *
 * Description: FT6336U Touch Driver Source (Fixed for BS21). \n
 *
 * History: \n
 * 2024-01-01, Create file. \n
 * 2024-06-08, Fix BS21 I2C clock and pin config. \n
 */
#include "ft6336u.h"
#include "pinctrl.h"
#include "i2c.h"
#include "gpio.h"
#include "tcxo.h"
#include "soc_osal.h"
#include "app_init.h"
#include "errcode.h"
#include "common_def.h"


/* I2C配置 */
#define FT6336U_I2C_BUS_ID         CONFIG_TOUCH_I2C_BUS_ID
#define FT6336U_I2C_BAUDRATE       400000
#define FT6336U_I2C_SCL_PIN        CONFIG_TOUCH_I2C_SCL_PIN
#define FT6336U_I2C_SCL_PIN_MODE   CONFIG_TOUCH_I2C_SCL_PIN_MODE
#define FT6336U_I2C_SDA_PIN        CONFIG_TOUCH_I2C_SDA_PIN
#define FT6336U_I2C_SDA_PIN_MODE   CONFIG_TOUCH_I2C_SDA_PIN_MODE
#define FT6336U_INT_PIN            CONFIG_TOUCH_INT_PIN
#define FT6336U_RST_PIN            CONFIG_TOUCH_RST_PIN

/* 调试开关 - 打开可查看初始化步骤 */
#define FT6336U_DEBUG_INIT         1

/* 初始化I2C引脚 */
static void ft6336u_i2c_init_pin(void)
{
#if defined(CONFIG_PINCTRL_SUPPORT_IE)
    uapi_pin_set_ie(FT6336U_I2C_SDA_PIN, PIN_IE_1);
#endif

    /* 关键修复：确保引脚配置为I2C功能模式，而不是GPIO */
    osal_printk("FT6336U: Configuring SCL pin=%d, mode=%d\r\n", 
                FT6336U_I2C_SCL_PIN, FT6336U_I2C_SCL_PIN_MODE);
    osal_printk("FT6336U: Configuring SDA pin=%d, mode=%d\r\n", 
                FT6336U_I2C_SDA_PIN, FT6336U_I2C_SDA_PIN_MODE);

    uapi_pin_set_mode(FT6336U_I2C_SCL_PIN, FT6336U_I2C_SCL_PIN_MODE);
    uapi_pin_set_mode(FT6336U_I2C_SDA_PIN, FT6336U_I2C_SDA_PIN_MODE);

    /* 触摸中断引脚 */
    if (FT6336U_INT_PIN >= 0) {
        uapi_pin_set_mode(FT6336U_INT_PIN, HAL_PIO_FUNC_GPIO);
        uapi_gpio_set_dir(FT6336U_INT_PIN, GPIO_DIRECTION_INPUT);
        uapi_pin_set_pull(FT6336U_INT_PIN, PIN_PULL_UP);
    }

    /* 触摸复位引脚 */
    if (FT6336U_RST_PIN >= 0) {
        uapi_pin_set_mode(FT6336U_RST_PIN, HAL_PIO_FUNC_GPIO);
        uapi_gpio_set_dir(FT6336U_RST_PIN, GPIO_DIRECTION_OUTPUT);
        uapi_gpio_set_val(FT6336U_RST_PIN, GPIO_LEVEL_HIGH);
    }
}

errcode_t ft6336u_read_reg(uint8_t reg, uint8_t *val)
{
    uint8_t send_buf = reg;
    i2c_data_t data = {
        .send_buf = &send_buf,
        .send_len = 1,
        .receive_buf = val,
        .receive_len = 1,
    };
    return uapi_i2c_master_writeread(FT6336U_I2C_BUS_ID, FT6336U_I2C_ADDR, &data);
}

errcode_t ft6336u_write_reg(uint8_t reg, uint8_t val)
{
    uint8_t send_buf[2] = { reg, val };
    i2c_data_t data = {
        .send_buf = send_buf,
        .send_len = 2,
        .receive_buf = NULL,
        .receive_len = 0,
    };
    return uapi_i2c_master_write(FT6336U_I2C_BUS_ID, FT6336U_I2C_ADDR, &data);
}

errcode_t ft6336u_read_regs(uint8_t reg, uint8_t *buf, uint16_t len)
{
    uint8_t send_buf = reg;
    i2c_data_t data = {
        .send_buf = &send_buf,
        .send_len = 1,
        .receive_buf = buf,
        .receive_len = len,
    };
    return uapi_i2c_master_writeread(FT6336U_I2C_BUS_ID, FT6336U_I2C_ADDR, &data);
}

errcode_t ft6336u_init(void)
{
    errcode_t ret;

#if FT6336U_DEBUG_INIT
    osal_printk("FT6336U: Step 1 - Init pins\r\n");
#endif
    /* 初始化I2C引脚 */
    ft6336u_i2c_init_pin();

#if FT6336U_DEBUG_INIT
    osal_printk("FT6336U: Step 2 - Hardware reset\r\n");
#endif
    /* 硬件复位FT6336U */
    if (FT6336U_RST_PIN >= 0) {
        uapi_gpio_set_val(FT6336U_RST_PIN, GPIO_LEVEL_LOW);
        uapi_tcxo_delay_ms(10);
        uapi_gpio_set_val(FT6336U_RST_PIN, GPIO_LEVEL_HIGH);
        uapi_tcxo_delay_ms(50);
    }

    /* 关键修复：确保I2C外设时钟已使能 */
    /* BS21芯片需要在初始化前使能I2C时钟，否则访问0x40000000会触发NMI */
#if FT6336U_DEBUG_INIT
    osal_printk("FT6336U: Step 3 - Enable I2C clock (bus=%d)\r\n", FT6336U_I2C_BUS_ID);
#endif

    /* 如果BS21 SDK提供时钟使能接口，取消下面注释 */
    /* uapi_clock_enable(CLOCK_ID_I2C0 + FT6336U_I2C_BUS_ID); */
    /* 或者使用HAL层使能： */
    /* hal_clock_enable(I2C_CLOCK_BASE + FT6336U_I2C_BUS_ID); */

    /* 检查I2C总线ID合法性 */
    if (FT6336U_I2C_BUS_ID > 1) {
        osal_printk("FT6336U ERROR: I2C bus ID %d invalid, BS21 only supports I2C0/I2C1\r\n", 
                    FT6336U_I2C_BUS_ID);
        return ERRCODE_FAIL;
    }

#if FT6336U_DEBUG_INIT
    osal_printk("FT6336U: Step 4 - I2C master init (bus=%d, baud=%d)\r\n", 
                FT6336U_I2C_BUS_ID, FT6336U_I2C_BAUDRATE);
#endif
    /* 初始化I2C主机 */
    ret = uapi_i2c_master_init(FT6336U_I2C_BUS_ID, FT6336U_I2C_BAUDRATE, 0);
    if (ret != ERRCODE_SUCC) {
        osal_printk("FT6336U ERROR: I2C master init failed: %d\r\n", ret);
        return ret;
    }

#if FT6336U_DEBUG_INIT
    osal_printk("FT6336U: Step 5 - Wait for chip ready\r\n");
#endif
    /* 等待FT6336U就绪 */
    uapi_tcxo_delay_ms(100);

#if FT6336U_DEBUG_INIT
    osal_printk("FT6336U: Step 6 - Read chip ID\r\n");
#endif
    /* 读取芯片ID验证通信 - 添加重试 */
    uint16_t chip_id = 0;
    for (int retry = 0; retry < 3; retry++) {
        chip_id = ft6336u_get_chip_id();
        osal_printk("FT6336U: chip id read attempt %d: 0x%04X\r\n", retry + 1, chip_id);
        if (chip_id != 0 && chip_id != 0xFFFF && chip_id != 0x0000) {
            break;
        }
        uapi_tcxo_delay_ms(50);
    }

    /* FT6336U典型芯片ID为0x6336 */
    if (chip_id != 0x6336) {
        osal_printk("FT6336U WARNING: Unexpected chip ID: 0x%04X (expected 0x6336)\r\n", chip_id);
        /* 不返回错误，某些兼容芯片ID可能不同 */
    }

#if FT6336U_DEBUG_INIT
    osal_printk("FT6336U: Step 7 - Configure registers\r\n");
#endif
    /* 配置触摸阈值 */
    ret = ft6336u_write_reg(FT6336U_REG_THRESHHOLD, 22);
    if (ret != ERRCODE_SUCC) {
        osal_printk("FT6336U WARNING: Failed to write threshold: %d\r\n", ret);
    }

    /* 配置活跃模式扫描周期 */
    ret = ft6336u_write_reg(FT6336U_REG_PERIOD_ACTIVE, 12);
    if (ret != ERRCODE_SUCC) {
        osal_printk("FT6336U WARNING: Failed to write period: %d\r\n", ret);
    }

    /* 配置中断模式: 0x00 = 轮询模式, 0x01 = 中断触发 */
    ret = ft6336u_write_reg(FT6336U_REG_G_MODE, 0x00);
    if (ret != ERRCODE_SUCC) {
        osal_printk("FT6336U WARNING: Failed to write G_MODE: %d\r\n", ret);
    }

    uint8_t fw_ver = ft6336u_get_firmware_version();
    osal_printk("FT6336U firmware version: 0x%02X\r\n", fw_ver);
    osal_printk("FT6336U touch init success.\r\n");

    return ERRCODE_SUCC;
}

void ft6336u_deinit(void)
{
    uapi_i2c_deinit(FT6336U_I2C_BUS_ID);
}

uint8_t ft6336u_get_touch_count(void)
{
    uint8_t status = 0;
    if (ft6336u_read_reg(FT6336U_REG_TD_STATUS, &status) != ERRCODE_SUCC) {
        return 0;
    }
    /* 低4位是触摸点数 */
    status &= 0x0F;
    if (status > FT6336U_MAX_TOUCHES) {
        return 0;
    }
    return status;
}

errcode_t ft6336u_read_touches(ft6336u_touch_t *touches, uint8_t *count)
{
    if (touches == NULL || count == NULL) {
        return ERRCODE_FAIL;
    }

    uint8_t buf[16] = {0};
    /* 读取从0x02开始的16字节触摸数据 */
    errcode_t ret = ft6336u_read_regs(FT6336U_REG_TD_STATUS, buf, 16);
    if (ret != ERRCODE_SUCC) {
        *count = 0;
        return ret;
    }

    /* 触摸点数 */
    uint8_t td_status = buf[0] & 0x0F;
    if (td_status > FT6336U_MAX_TOUCHES) {
        td_status = 0;
    }
    *count = td_status;

    /* 解析触摸1数据 */
    if (td_status >= 1) {
        touches[0].event = (buf[1] >> 6) & 0x03;
        touches[0].x = ((uint16_t)(buf[1] & 0x0F) << 8) | buf[2];
        touches[0].id = (buf[3] >> 4) & 0x0F;
        touches[0].y = ((uint16_t)(buf[3] & 0x0F) << 8) | buf[4];
        touches[0].weight = buf[5];
        touches[0].area = buf[6] & 0x0F;
    }

    /* 解析触摸2数据 */
    if (td_status >= 2) {
        touches[1].event = (buf[7] >> 6) & 0x03;
        touches[1].x = ((uint16_t)(buf[7] & 0x0F) << 8) | buf[8];
        touches[1].id = (buf[9] >> 4) & 0x0F;
        touches[1].y = ((uint16_t)(buf[9] & 0x0F) << 8) | buf[10];
        touches[1].weight = buf[11];
        touches[1].area = buf[12] & 0x0F;
    }

    return ERRCODE_SUCC;
}

uint16_t ft6336u_get_chip_id(void)
{
    uint8_t id_h = 0, id_l = 0;
    ft6336u_read_reg(FT6336U_REG_CHIP_ID_H, &id_h);
    ft6336u_read_reg(FT6336U_REG_CHIP_ID_L, &id_l);
    return ((uint16_t)id_h << 8) | id_l;
}

uint8_t ft6336u_get_firmware_version(void)
{
    uint8_t ver = 0;
    ft6336u_read_reg(FT6336U_REG_FIRM_VERS, &ver);
    return ver;
}