/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
  (C)2018 Semtech

Description:
    TODO

License: Revised BSD License, see LICENSE.TXT file include in the project
*/


/* -------------------------------------------------------------------------- */
/* --- DEPENDANCIES --------------------------------------------------------- */

#include <stdint.h>     /* C99 types */
#include <stdio.h>      /* printf fprintf */
#include <stdlib.h>     /* malloc free */
#include <unistd.h>     /* lseek, close */
#include <fcntl.h>      /* open */
#include <string.h>     /* memset */

#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

#include "loragw_spi.h"
#include "loragw_reg.h"
#include "loragw_aux.h"
#include "loragw_sx1250.h"

/* -------------------------------------------------------------------------- */
/* --- PRIVATE MACROS ------------------------------------------------------- */

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#if DEBUG_RAD == 1
    #define DEBUG_MSG(str)                fprintf(stderr, str)
    #define DEBUG_PRINTF(fmt, args...)    fprintf(stderr,"%s:%d: "fmt, __FUNCTION__, __LINE__, args)
    #define CHECK_NULL(a)                if(a==NULL){fprintf(stderr,"%s:%d: ERROR: NULL POINTER AS ARGUMENT\n", __FUNCTION__, __LINE__);return LGW_SPI_ERROR;}
#else
    #define DEBUG_MSG(str)
    #define DEBUG_PRINTF(fmt, args...)
    #define CHECK_NULL(a)                if(a==NULL){return LGW_SPI_ERROR;}
#endif

#define SX1250_FREQ_TO_REG(f)       (uint32_t)((uint64_t)f * (1 << 25) / 32000000U)

/* -------------------------------------------------------------------------- */
/* --- PRIVATE CONSTANTS ---------------------------------------------------- */

#define WAIT_BUSY_SX1250_MS  1

/* -------------------------------------------------------------------------- */
/* --- INTERNAL SHARED VARIABLES -------------------------------------------- */

extern void *lgw_spi_target; /*! generic pointer to the SPI device */

/* -------------------------------------------------------------------------- */
/* --- PUBLIC FUNCTIONS DEFINITION ------------------------------------------ */

int sx1250_write_command(uint8_t rf_chain, sx1250_op_code_t op_code, uint8_t *data, uint16_t size) {
    int spi_device;
    int cmd_size = 2; /* header + op_code */
    uint8_t out_buf[cmd_size + size];
    uint8_t command_size;
    struct spi_ioc_transfer k;
    int a, i;

    /* wait BUSY */
    wait_ms(WAIT_BUSY_SX1250_MS);

    /* check input variables */
    CHECK_NULL(lgw_spi_target);

    spi_device = *(int *)lgw_spi_target; /* must check that spi_target is not null beforehand */

    /* prepare frame to be sent */
    out_buf[0] = (rf_chain == 0) ? LGW_SPI_MUX_TARGET_RADIOA : LGW_SPI_MUX_TARGET_RADIOB;
    out_buf[1] = (uint8_t)op_code;
    for(i = 0; i < (int)size; i++) {
        out_buf[cmd_size + i] = data[i];
    }
    command_size = cmd_size + size;

    /* I/O transaction */
    memset(&k, 0, sizeof(k)); /* clear k */
    k.tx_buf = (unsigned long) out_buf;
    k.len = command_size;
    k.speed_hz = SPI_SPEED;
    k.cs_change = 0;
    k.bits_per_word = 8;
    a = ioctl(spi_device, SPI_IOC_MESSAGE(1), &k);

    /* determine return code */
    if (a != (int)k.len) {
        DEBUG_MSG("ERROR: SPI WRITE FAILURE\n");
        return LGW_SPI_ERROR;
    } else {
        DEBUG_MSG("Note: SPI write success\n");
        return LGW_SPI_SUCCESS;
    }
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

int sx1250_read_command(uint8_t rf_chain, sx1250_op_code_t op_code, uint8_t *data, uint16_t size) {
    int spi_device;
    int cmd_size = 2; /* header + op_code + NOP */
    uint8_t out_buf[cmd_size + size];
    uint8_t command_size;
    uint8_t in_buf[ARRAY_SIZE(out_buf)];
    struct spi_ioc_transfer k;
    int a, i;

    /* wait BUSY */
    wait_ms(WAIT_BUSY_SX1250_MS);

    /* check input variables */
    CHECK_NULL(lgw_spi_target);
    CHECK_NULL(data);

    spi_device = *(int *)lgw_spi_target; /* must check that spi_target is not null beforehand */

    /* prepare frame to be sent */
    out_buf[0] = (rf_chain == 0) ? LGW_SPI_MUX_TARGET_RADIOA : LGW_SPI_MUX_TARGET_RADIOB;
    out_buf[1] = (uint8_t)op_code;
    for(i = 0; i < (int)size; i++) {
        out_buf[cmd_size + i] = data[i];
    }
    command_size = cmd_size + size;

    /* I/O transaction */
    memset(&k, 0, sizeof(k)); /* clear k */
    k.tx_buf = (unsigned long) out_buf;
    k.rx_buf = (unsigned long) in_buf;
    k.len = command_size;
    k.cs_change = 0;
    a = ioctl(spi_device, SPI_IOC_MESSAGE(1), &k);

    /* determine return code */
    if (a != (int)k.len) {
        DEBUG_MSG("ERROR: SPI READ FAILURE\n");
        return LGW_SPI_ERROR;
    } else {
        DEBUG_MSG("Note: SPI read success\n");
        //*data = in_buf[command_size - 1];
        memcpy(data, in_buf + cmd_size, size);
        return LGW_SPI_SUCCESS;
    }
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

int sx1250_setup(uint8_t rf_chain, uint32_t freq_hz) {
    int32_t freq_reg;
    uint8_t buff[16];

    /* Set Radio in Standby mode */
    buff[0] = (uint8_t)STDBY_XOSC;
    sx1250_write_command(rf_chain, SET_STANDBY, buff, 1);
    buff[0] = 0x00;
    sx1250_read_command(rf_chain, GET_STATUS, buff, 1);
    printf("%s: get_status: 0x%02X\n", __FUNCTION__, buff[0]);

    /* Configure DIO for Rx */
    buff[0] = 0x05;
    buff[1] = 0x82;
    buff[2] = 0x00;
    sx1250_write_command(rf_chain, WRITE_REGISTER, buff, 3); /* Drive strength to min */
    buff[0] = 0x05;
    buff[1] = 0x83;
    buff[2] = 0x00;
    sx1250_write_command(rf_chain, WRITE_REGISTER, buff, 3); /* Input enable, all disabled */
    buff[0] = 0x05;
    buff[1] = 0x84;
    buff[2] = 0x00;
    sx1250_write_command(rf_chain, WRITE_REGISTER, buff, 3); /* No pull up */
    buff[0] = 0x05;
    buff[1] = 0x85;
    buff[2] = 0x00;
    sx1250_write_command(rf_chain, WRITE_REGISTER, buff, 3); /* No pull down */
    buff[0] = 0x05;
    buff[1] = 0x80;
    buff[2] = 0x00;
    sx1250_write_command(rf_chain, WRITE_REGISTER, buff, 3); /* Output enable, all enabled */

    /* TODO ?? */
    buff[0] = 0x08;
    buff[1] = 0xB6;
    buff[2] = 0x2A;
    sx1250_write_command(rf_chain, WRITE_REGISTER, buff, 3); /* Fix gain 10 */

    /* Set frequency */
    freq_reg = SX1250_FREQ_TO_REG(freq_hz);
    buff[0] = (uint8_t)(freq_reg >> 24);
    buff[1] = (uint8_t)(freq_reg >> 16);
    buff[2] = (uint8_t)(freq_reg >> 8);
    buff[3] = (uint8_t)(freq_reg >> 0);
    sx1250_write_command(rf_chain, SET_RF_FREQUENCY, buff, 4);

    /* Set frequency offset to 0 */
    buff[0] = 0x08;
    buff[1] = 0x8F;
    buff[2] = 0x00;
    buff[3] = 0x00;
    buff[4] = 0x00;
    sx1250_write_command(rf_chain, WRITE_REGISTER, buff, 5);

    /* Set Radio in Rx mode, necessary to give a clock to SX1302 */
    buff[0] = 0xFF;
    buff[1] = 0xFF;
    buff[2] = 0xFF;
    sx1250_write_command(rf_chain, SET_RX, buff, 3); /* Rx Continuous */

    buff[0] = 0x05;
    buff[1] = 0x87;
    buff[2] = 0x08;
    sx1250_write_command(rf_chain, WRITE_REGISTER, buff, 3); /* FPGA_MODE_RX */

    return 0;
}

/* --- EOF ------------------------------------------------------------------ */