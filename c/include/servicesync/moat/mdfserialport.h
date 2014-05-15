/*
 * LEGAL NOTICE
 *
 * Copyright (C) 2014 InventIt Inc. All rights reserved.
 *
 * This source code, product and/or document is protected under licenses 
 * restricting its use, copying, distribution, and decompilation.
 * No part of this source code, product or document may be reproduced in
 * any form by any means without prior written authorization of InventIt Inc.
 * and its licensors, if any.
 *
 * InventIt Inc.
 * 9F ATOM KOJIMACHI TOWER
 * 4-4-7 Kojimachi, Chiyoda-ku, Tokyo 102-0083
 * JAPAN
 * http://www.yourinventit.com/
 */


#ifndef MDFSERIALPORT_H_
#define MDFSERIALPORT_H_


SSE_BEGIN_C_DECLS

typedef struct MDFSerialPort_ MDFSerialPort;

enum mdf_serial_baud_rate_ {
  MDF_SERIAL_BR_0,
  MDF_SERIAL_BR_110,
  MDF_SERIAL_BR_300,
  MDF_SERIAL_BR_600,
  MDF_SERIAL_BR_1200,
  MDF_SERIAL_BR_2400,
  MDF_SERIAL_BR_4800,
  MDF_SERIAL_BR_9600,
  MDF_SERIAL_BR_19200,
  MDF_SERIAL_BR_38400,
  MDF_SERIAL_BR_56000,
  MDF_SERIAL_BR_57600,
  MDF_SERIAL_BR_115200,
  MDF_SERIAL_BR_128000,
  MDF_SERIAL_BR_230400,
  MDF_SERIAL_BR_256000,
  MDF_SERIAL_BR_460800,
  MDF_SERIAL_BR_500000,
  MDF_SERIAL_BRs
};

enum mdf_serial_data_bits_ {
  MDF_SERIAL_DB_5,
  MDF_SERIAL_DB_6,
  MDF_SERIAL_DB_7,
  MDF_SERIAL_DB_8,
  MDF_SERIAL_DBs
};

enum mdf_serial_stop_bits_ {
  MDF_SERIAL_SB_1,
  MDF_SERIAL_SB_1_5,
  MDF_SERIAL_SB_2,
  MDF_SERIAL_SBs
};

enum mdf_serial_parity_ {
  MDF_SERIAL_PARITY_NONE,
  MDF_SERIAL_PARITY_ODD,
  MDF_SERIAL_PARITY_EVEN,
  MDF_SERIAL_PARITYs
};

typedef struct MDFSerialAttributes_ MDFSerialAttributes;
struct MDFSerialAttributes_ {
  sse_uint BaudRate;    /* enum mdf_serial_baud_rate_ */
  sse_uint DataBits; /* enum mdf_serial_data_bits_ */
  sse_uint StopBits; /* enum mdf_serial_stop_bits_ */
  sse_uint Parity;   /* enum mdf_serial_parity_ */
  sse_bool CTSEnabled;
  sse_bool DTREnabled;
  sse_bool XOnEnabled;
};

MDFSerialPort * mdf_serial_port_new(sse_char *in_port_name);
void mdf_serial_port_free(MDFSerialPort *self);
sse_int mdf_serial_port_get_attributes(MDFSerialPort *self, MDFSerialAttributes *out_attr);
sse_int mdf_serial_port_set_attributes(MDFSerialPort *self, MDFSerialAttributes *in_attr);
sse_char * mdf_serial_port_get_name(MDFSerialPort *self);

SSE_END_C_DECLS

#endif /* MDFSERIALPORT_H_ */
