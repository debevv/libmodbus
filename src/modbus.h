/*
 * Copyright © 2001-2010 Stéphane Raimbault <stephane.raimbault@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser Public License for more details.
 *
 * You should have received a copy of the GNU Lesser Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _MODBUS_H_
#define _MODBUS_H_

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#include <termios.h>
#if defined(__FreeBSD__ ) && __FreeBSD__ < 5
#include <netinet/in_systm.h>
#endif
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include <modbus/version.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MODBUS_TCP_DEFAULT_PORT   502
#define MODBUS_BROADCAST_ADDRESS  255

/* Slave index */
#define HEADER_LENGTH_RTU           1
#define PRESET_QUERY_LENGTH_RTU     6
#define PRESET_RESPONSE_LENGTH_RTU  2

#define HEADER_LENGTH_TCP           7
#define PRESET_QUERY_LENGTH_TCP    12
#define PRESET_RESPONSE_LENGTH_TCP  8

#define CHECKSUM_LENGTH_RTU         2
#define CHECKSUM_LENGTH_TCP         0

/* It's not really the minimal length (the real one is report slave ID
 * in RTU (4 bytes)) but it's a convenient size to use in RTU or TCP
 * communications to read many values or write a single one.
 * Maximum between :
 * - HEADER_LENGTH_TCP (7) + function (1) + address (2) + number (2)
 * - HEADER_LENGTH_RTU (1) + function (1) + address (2) + number (2) + CRC (2)
*/
#define MIN_QUERY_LENGTH           12

/* Modbus_Application_Protocol_V1_1b.pdf Chapter 4 Section 1 Page 5:
 *  - RS232 / RS485 ADU = 253 bytes + slave (1 byte) + CRC (2 bytes) = 256 bytes
 *  - TCP MODBUS ADU = 253 bytes + MBAP (7 bytes) = 260 bytes
 */
#define MAX_PDU_LENGTH            253
#define MAX_ADU_LENGTH_RTU        256
#define MAX_ADU_LENGTH_TCP        260

/* Kept for compatibility reasons (deprecated) */
#define MAX_MESSAGE_LENGTH        260

#define EXCEPTION_RESPONSE_LENGTH_RTU  5

/* Modbus_Application_Protocol_V1_1b.pdf (chapter 6 section 1 page 12)
 * Quantity of Coils (2 bytes): 1 to 2000 (0x7D0)
 */
#define MAX_STATUS               2000

/* Modbus_Application_Protocol_V1_1b.pdf (chapter 6 section 3 page 15)
 * Quantity of Registers (2 bytes): 1 to 125 (0x7D)
 */
#define MAX_REGISTERS             125

#define REPORT_SLAVE_ID_LENGTH     75

/* Time out between trames in microsecond */
#define TIME_OUT_BEGIN_OF_TRAME 500000
#define TIME_OUT_END_OF_TRAME   500000

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef OFF
#define OFF 0
#endif

#ifndef ON
#define ON 1
#endif

/* Function codes */
#define FC_READ_COIL_STATUS          0x01  /* discretes inputs */
#define FC_READ_INPUT_STATUS         0x02  /* discretes outputs */
#define FC_READ_HOLDING_REGISTERS    0x03
#define FC_READ_INPUT_REGISTERS      0x04
#define FC_FORCE_SINGLE_COIL         0x05
#define FC_PRESET_SINGLE_REGISTER    0x06
#define FC_READ_EXCEPTION_STATUS     0x07
#define FC_FORCE_MULTIPLE_COILS      0x0F
#define FC_PRESET_MULTIPLE_REGISTERS 0x10
#define FC_REPORT_SLAVE_ID           0x11

/* Random number to avoid errno conflicts */
#define MODBUS_ENOBASE 112345678

/* Protocol exceptions */
enum {
        MODBUS_EXCEPTION_ILLEGAL_FUNCTION = 0x01,
        MODBUS_EXCEPTION_ILLEGAL_DATA_ADDRESS,
        MODBUS_EXCEPTION_ILLEGAL_DATA_VALUE,
        MODBUS_EXCEPTION_SLAVE_OR_SERVER_FAILURE,
        MODBUS_EXCEPTION_ACKNOWLEDGE,
        MODBUS_EXCEPTION_SLAVE_OR_SERVER_BUSY,
        MODBUS_EXCEPTION_NEGATIVE_ACKNOWLEDGE,
        MODBUS_EXCEPTION_MEMORY_PARITY,
        MODBUS_EXCEPTION_NOT_DEFINED,
        MODBUS_EXCEPTION_GATEWAY_PATH,
        MODBUS_EXCEPTION_GATEWAY_TARGET,
        MODBUS_EXCEPTION_MAX
};

#define EMBXILFUN  (MODBUS_ENOBASE + MODBUS_EXCEPTION_ILLEGAL_FUNCTION)
#define EMBXILADD  (MODBUS_ENOBASE + MODBUS_EXCEPTION_ILLEGAL_DATA_ADDRESS)
#define EMBXILVAL  (MODBUS_ENOBASE + MODBUS_EXCEPTION_ILLEGAL_DATA_VALUE)
#define EMBXSFAIL  (MODBUS_ENOBASE + MODBUS_EXCEPTION_SLAVE_OR_SERVER_FAILURE)
#define EMBXACK    (MODBUS_ENOBASE + MODBUS_EXCEPTION_ACKNOWLEDGE)
#define EMBXSBUSY  (MODBUS_ENOBASE + MODBUS_EXCEPTION_SLAVE_OR_SERVER_BUSY)
#define EMBXNACK   (MODBUS_ENOBASE + MODBUS_EXCEPTION_NEGATIVE_ACKNOWLEDGE)
#define EMBXMEMPAR (MODBUS_ENOBASE + MODBUS_EXCEPTION_MEMORY_PARITY)
#define EMBXGPATH  (MODBUS_ENOBASE + MODBUS_EXCEPTION_GATEWAY_PATH)
#define EMBXGTAR   (MODBUS_ENOBASE + MODBUS_EXCEPTION_GATEWAY_TARGET)

/* Native libmodbus error codes */
#define EMBBADCRC  (EMBXGTAR + 1)
#define EMBBADDATA (EMBXGTAR + 2)
#define EMBBADEXC  (EMBXGTAR + 3)
#define EMBUNKEXC  (EMBXGTAR + 4)
#define EMBMDATA   (EMBXGTAR + 5)

/* Internal using */
#define MSG_LENGTH_UNDEFINED -1

typedef enum { RTU=0, TCP } type_com_t;

/* This structure is byte-aligned */
typedef struct {
        /* Slave address */
        int slave;
        /* Descriptor (tty or socket) */
        int fd;
        /* Communication mode: RTU or TCP */
        type_com_t type_com;
        /* Flag debug */
        int debug;
        /* TCP port */
        int port;
        /* Device: "/dev/ttyS0", "/dev/ttyUSB0" or "/dev/tty.USA19*"
           on Mac OS X for KeySpan USB<->Serial adapters this string
           had to be made bigger on OS X as the directory+file name
           was bigger than 19 bytes. Making it 67 bytes for now, but
           OS X does support 256 byte file names. May become a problem
           in the future. */
#ifdef __APPLE_CC__
        char device[64];
#else
        char device[16];
#endif
        /* Bauds: 9600, 19200, 57600, 115200, etc */
        int baud;
        /* Data bit */
        uint8_t data_bit;
        /* Stop bit */
        uint8_t stop_bit;
        /* Parity: "even", "odd", "none" */
        char parity[5];
        /* In error_treat with TCP, do a reconnect or just dump the error */
        uint8_t error_recovery;
        /* IP address */
        char ip[16];
        /* Save old termios settings */
        struct termios old_tios;
} modbus_param_t;

typedef struct {
        int nb_coil_status;
        int nb_input_status;
        int nb_input_registers;
        int nb_holding_registers;
        uint8_t *tab_coil_status;
        uint8_t *tab_input_status;
        uint16_t *tab_input_registers;
        uint16_t *tab_holding_registers;
} modbus_mapping_t;

void modbus_init_rtu(modbus_param_t *mb_param, const char *device,
                     int baud, const char *parity, int data_bit,
                     int stop_bit, int slave);
void modbus_init_tcp(modbus_param_t *mb_param, const char *ip_address, int port,
                     int slave);
void modbus_set_slave(modbus_param_t *mb_param, int slave);
int modbus_set_error_recovery(modbus_param_t *mb_param, int enabled);

int modbus_connect(modbus_param_t *mb_param);
void modbus_close(modbus_param_t *mb_param);

int modbus_flush(modbus_param_t *mb_param);
void modbus_set_debug(modbus_param_t *mb_param, int boolean);

const char *modbus_strerror(int errnum);

int read_coil_status(modbus_param_t *mb_param, int start_addr, int nb,
                     uint8_t *dest);
int read_input_status(modbus_param_t *mb_param, int start_addr, int nb,
                      uint8_t *dest);
int read_holding_registers(modbus_param_t *mb_param, int start_addr, int nb,
                           uint16_t *dest);
int read_input_registers(modbus_param_t *mb_param, int start_addr, int nb,
                         uint16_t *dest);
int force_single_coil(modbus_param_t *mb_param, int coil_addr, int state);

int preset_single_register(modbus_param_t *mb_param, int reg_addr, int value);
int force_multiple_coils(modbus_param_t *mb_param, int start_addr, int nb,
                         const uint8_t *data);
int preset_multiple_registers(modbus_param_t *mb_param, int start_addr, int nb,
                              const uint16_t *data);
int report_slave_id(modbus_param_t *mb_param, uint8_t *dest);

int modbus_mapping_new(modbus_mapping_t *mb_mapping,
                       int nb_coil_status, int nb_input_status,
                       int nb_holding_registers, int nb_input_registers);
void modbus_mapping_free(modbus_mapping_t *mb_mapping);

int modbus_slave_listen_tcp(modbus_param_t *mb_param, int nb_connection);
int modbus_slave_accept_tcp(modbus_param_t *mb_param, int *socket);
int modbus_slave_receive(modbus_param_t *mb_param, int sockfd, uint8_t *query);
int modbus_slave_manage(modbus_param_t *mb_param, const uint8_t *query,
                        int query_length, modbus_mapping_t *mb_mapping);
void modbus_slave_close_tcp(int socket);

/**
 * UTILS FUNCTIONS
 **/

void set_bits_from_byte(uint8_t *dest, int address, const uint8_t value);
void set_bits_from_bytes(uint8_t *dest, int address, unsigned int nb_bits,
                         const uint8_t *tab_byte);
uint8_t get_byte_from_bits(const uint8_t *src, int address, unsigned int nb_bits);
float modbus_read_float(const uint16_t *src);
void modbus_write_float(float real, uint16_t *dest);

#ifdef __cplusplus
}
#endif

#endif  /* _MODBUS_H_ */