/*
 * LEGAL NOTICE
 *
 * Copyright (C) 2013 InventIt Inc. All rights reserved.
 *
 * This source code, product and/or document is protected under licenses 
 * restricting its use, copying, distribution, and decompilation.
 * No part of this source code, product or document may be reproduced in
 * any form by any means without prior written authorization of InventIt Inc.
 * and its licensors, if any.
 *
 * InventIt Inc.
 * 9F KOJIMACHI CP BUILDING
 * 4-4-7 Kojimachi, Chiyoda-ku, Tokyo 102-0083
 * JAPAN
 * http://www.yourinventit.com/
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <ev.h>


#include <servicesync/moat.h>
#include "io.h"
#include "serial_port.h"

struct MDFSerialPort_ {
  MDFIO Base;
};

static sse_int
termios_to_serial_attributes(struct termios *in_t, MDFSerialAttributes *out_attr)
{
  speed_t t_speed;
  int speed;

  sse_memset(out_attr, 0, sizeof(MDFSerialAttributes));
  t_speed = cfgetispeed(in_t);
  switch(t_speed) {
  case B0:
    speed = MDF_SERIAL_BR_0;
    break;
  case B110:
    speed = MDF_SERIAL_BR_110;
    break;
  case B300:
    speed = MDF_SERIAL_BR_300;
    break;
  case B600:
    speed = MDF_SERIAL_BR_600;
    break;
  case B1200:
    speed = MDF_SERIAL_BR_1200;
    break;
  case B2400:
    speed = MDF_SERIAL_BR_2400;
    break;
  case B4800:
    speed = MDF_SERIAL_BR_4800;
    break;
  case B9600:
    speed = MDF_SERIAL_BR_9600;
    break;
  case B19200:
    speed = MDF_SERIAL_BR_19200;
    break;
  case B38400:
    speed = MDF_SERIAL_BR_38400;
    break;
  case B57600:
    speed = MDF_SERIAL_BR_57600;
    break;
  case B115200:
    speed = MDF_SERIAL_BR_115200;
    break;
  case B230400:
    speed = MDF_SERIAL_BR_230400;
    break;
  case B460800:
    speed = MDF_SERIAL_BR_460800;
    break;
  case B500000:
    speed = MDF_SERIAL_BR_500000;
    break;
  default:
    /* not supported */
    return SSE_E_INVAL;
  }
  switch (in_t->c_cflag & CSIZE) {
  case CS5:
    out_attr->DataBits = MDF_SERIAL_DB_5;
    break;
  case CS6:
    out_attr->DataBits = MDF_SERIAL_DB_6;
    break;
  case CS7:
    out_attr->DataBits = MDF_SERIAL_DB_7;
    break;
  case CS8:
    out_attr->DataBits = MDF_SERIAL_DB_8;
    break;
  default:
    return SSE_E_INVAL;
  }
  if (in_t->c_cflag & CSTOPB) {
    out_attr->StopBits = MDF_SERIAL_SB_2;
  } else {
    out_attr->StopBits = MDF_SERIAL_SB_1;
  }
  if (in_t->c_cflag & PARENB) {
    if (in_t->c_cflag & PARODD) {
      out_attr->Parity = MDF_SERIAL_PARITY_ODD;
    } else {
      out_attr->Parity = MDF_SERIAL_PARITY_EVEN;
    }
  } else {
    out_attr->Parity = MDF_SERIAL_PARITY_NONE;
  }
  if (in_t->c_iflag & (IXON | IXOFF)) {
    out_attr->XOnEnabled = sse_true;
  }
  if (in_t->c_cflag & CRTSCTS) {
    out_attr->CTSEnabled = sse_true;
  }
  return SSE_E_OK;
}

static sse_int
mdf_serial_attributes_to_termios(MDFSerialAttributes *in_attr, struct termios *inout_t)
{
  speed_t speed;
  int data_bits;

  switch(in_attr->BaudRate) {
  case MDF_SERIAL_BR_0:
    speed = B0;
    break;
  case MDF_SERIAL_BR_110:
    speed = B110;
    break;
  case MDF_SERIAL_BR_300:
    speed = B300;
    break;
  case MDF_SERIAL_BR_600:
    speed = B600;
    break;
  case MDF_SERIAL_BR_1200:
    speed = B1200;
    break;
  case MDF_SERIAL_BR_2400:
    speed = B2400;
    break;
  case MDF_SERIAL_BR_4800:
    speed = B4800;
    break;
  case MDF_SERIAL_BR_9600:
    speed = B9600;
    break;
  case MDF_SERIAL_BR_19200:
    speed = B19200;
    break;
  case MDF_SERIAL_BR_38400:
    speed = B38400;
    break;
  case MDF_SERIAL_BR_57600:
    speed = B57600;
    break;
  case MDF_SERIAL_BR_115200:
    speed = B115200;
    break;
  case MDF_SERIAL_BR_230400:
    speed = B230400;
    break;
  case MDF_SERIAL_BR_460800:
    speed = B460800;
    break;
  case MDF_SERIAL_BR_500000:
    speed = B500000;
    break;
  case MDF_SERIAL_BR_56000:
  case MDF_SERIAL_BR_128000:
  case MDF_SERIAL_BR_256000:
  default:
    /* not supported */
    return SSE_E_INVAL;
  }
  switch (in_attr->DataBits) {
  case MDF_SERIAL_DB_5:
    data_bits = CS5;
    break;
  case MDF_SERIAL_DB_6:
    data_bits = CS6;
    break;
  case MDF_SERIAL_DB_7:
    data_bits = CS7;
    break;
  case MDF_SERIAL_DB_8:
    data_bits = CS8;
    break;
  default:
    return SSE_E_INVAL;
  }
  cfsetospeed(inout_t, speed);
  cfsetispeed(inout_t, speed);
  inout_t->c_cflag = (inout_t->c_cflag & ~CSIZE) | data_bits;
  inout_t->c_lflag = 0;
  inout_t->c_oflag = 0;
  inout_t->c_cc[VMIN]  = 1;
  inout_t->c_cc[VTIME] = 0;
  inout_t->c_cflag |= (CLOCAL | CREAD);
  if (in_attr->Parity == MDF_SERIAL_PARITY_NONE) {
    inout_t->c_cflag &= ~(PARENB | PARODD);
  } else if (in_attr->Parity == MDF_SERIAL_PARITY_ODD) {
    inout_t->c_cflag |= (PARENB | PARODD);
  } else if (in_attr->Parity == MDF_SERIAL_PARITY_EVEN) {
    inout_t->c_cflag |= PARENB;
  }
  if (in_attr->StopBits == MDF_SERIAL_SB_1) {
    inout_t->c_cflag &= ~CSTOPB;
  } else {
    inout_t->c_cflag |= CSTOPB;
  }
  if (in_attr->CTSEnabled) {
    inout_t->c_cflag |= CRTSCTS;
  } else {
    inout_t->c_cflag &= ~CRTSCTS;
  }
  if (in_attr->XOnEnabled) {
    inout_t->c_iflag |= (IXON | IXOFF);
  } else {
    inout_t->c_iflag &= ~(IXON | IXOFF);
  }
  inout_t->c_iflag &= ~IXANY;
  return SSE_E_OK;
}

sse_int
mdf_serial_port_get_attributes(MDFSerialPort *self, MDFSerialAttributes *out_attr)
{
  struct termios t;
  int err;

  err = tcgetattr(self->Base.Desc, &t);
  if (err) {
    return SSE_E_GENERIC;
  }
  err = termios_to_serial_attributes(&t, out_attr);
  if (err) {
    return err;
  }
  return SSE_E_OK;
}

sse_int
mdf_serial_port_set_attributes(MDFSerialPort *self, MDFSerialAttributes *in_attr)
{
  struct termios t;
  int err;

  err = tcgetattr(self->Base.Desc, &t);
  if (err) {
    return SSE_E_GENERIC;
  }
  err = mdf_serial_attributes_to_termios(in_attr, &t);
  if (err) {
    return err;
  }
  err = tcsetattr(self->Base.Desc, TCSANOW, &t);
  if (err) {
    return SSE_E_GENERIC;
  }
  return SSE_E_OK;
}

MDFSerialPort *
mdf_serial_port_new(sse_char *in_port_name)
{
  MDFSerialPort *p = NULL;
  sse_char *path = NULL;
  sse_int err;

  p = sse_zeroalloc(sizeof(MDFSerialPort));
  if (p == NULL) {
    goto error_exit;
  }
  err = mdf_io_init(&p->Base);
  if (err) {
    goto error_exit;
  }
  path = sse_strdup(in_port_name);
  if (path == NULL) {
    goto error_exit;
  }
  p->Base.Path = path;
  return p;

error_exit:
  if (path != NULL) {
    sse_free(path);
  }
  if (p != NULL) {
    sse_free(p);
  }
  return NULL;
}

void
mdf_serial_port_free(MDFSerialPort *self)
{
  mdf_io_fini(&self->Base);
  sse_free(self->Base.Path);
  sse_free(self);
}
