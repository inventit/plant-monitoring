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

#include <fcntl.h>
#include <servicesync/moat.h>
#include "device_framework.h"
#include "data_collector.h"

#define DC_DEFAULT_BAUDRATE MDF_SERIAL_BR_57600
#define DC_BUFFER_SIZE  (256)
#define DC_VER_CMD_HEADER_LEN   (5)
#define DC_ERR_CMD_HEADER_LEN   (5)
#define DC_DATA_CMD_HEADER_LEN   (6)

#define DC_LOG_ERROR(format, ...)   MOAT_LOG_ERROR("Collector", format, ##__VA_ARGS__)
#define DC_LOG_INFO(format, ...)    MOAT_LOG_INFO("Collector", format, ##__VA_ARGS__)
#define DC_LOG_DEBUG(format, ...)   MOAT_LOG_DEBUG("Collector", format, ##__VA_ARGS__)
#define DC_LOG_TRACE(format, ...)   MOAT_LOG_TRACE("Collector", format, ##__VA_ARGS__)
#define DC_ENTER()      DC_LOG_TRACE("enter")
#define DC_LEAVE()      DC_LOG_TRACE("leave")

struct TDataCollector_ {
  Moat fMoat;
  sse_char *fUploadSensingDataId;
  MoatObject *fConf;
  MDFDeviceFinder *fDeviceFinder;
  MDFSerialPort *fSerialPort;
  sse_char *fSerialNumber;
  sse_uint fReadBytes;
  sse_char fBuffer[DC_BUFFER_SIZE];
  MoatObject *fSensingData;
  MoatTimer *fTimer;
  sse_int fUploadTimerId;
};

static sse_int
TDataCollector_UploadSensingData(TDataCollector *self)
{
  MoatObject *d = self->fSensingData;
  sse_int len;
  sse_int req_id;

  len = moat_object_get_length(d);
  if (len == 0) {
    DC_LOG_DEBUG("no sensing data.");
    return SSE_E_NOENT;
  }
  req_id = moat_send_notification(self->fMoat, self->fUploadSensingDataId, NULL, "SensingData", d, NULL, NULL);
  DC_LOG_DEBUG("noti req_id:[%d]", req_id);
  moat_object_remove_all(d);
  return SSE_E_OK;
}

static sse_bool
DataCollector_UploadIntervalProc(sse_int in_timer_id, sse_pointer in_user_data)
{
  TDataCollector *dc = (TDataCollector *)in_user_data;
  sse_int err;

  err = TDataCollector_UploadSensingData(dc);
  if (err) {
    DC_LOG_ERROR("failed to upload.");
  }
  return sse_true;
}

static MoatObject *
DataCollector_CreateSensorData(sse_char *in_serial, sse_char *in_json_data)
{
  sse_int len;
  MoatObject *data = NULL;
  sse_char *err_msg = NULL;
  sse_uint64 ts;
  sse_int err;

  len = sse_strlen(in_json_data);
  err = moat_json_string_to_moat_object(in_json_data, len, &data, &err_msg);
  if (err) {
    DC_LOG_ERROR("failed to convert json to moat object. err=[%s]", err_msg);
    goto error_exit;
  }
  len = sse_strlen(in_serial);
  err = moat_object_add_string_value(data, "serialNumber", in_serial, len, sse_true, sse_true);
  if (err) {
    DC_LOG_ERROR("failed to add 'serialNumber'");
    goto error_exit;
  }
  ts = moat_get_timestamp_msec();
  err = moat_object_add_int64_value(data, "timestamp", ts, sse_true);
  if (err) {
    DC_LOG_ERROR("failed to add 'timestamp'");
    goto error_exit;
  }
  return data;

error_exit:
  if (err_msg != NULL) {
    sse_free(err_msg);
  }
  if (data != NULL) {
    moat_object_free(data);
  }
  return NULL;
}

static sse_int
TDataCollector_AddSensingData(TDataCollector *self, sse_char *in_str)
{
  MoatObject *data;
  MoatUUID uuid;
  sse_char uuid_str[MOAT_UUID_STRING_BUF_SIZE];
  sse_int err;

  data = DataCollector_CreateSensorData(self->fSerialNumber, in_str);
  if (data == NULL) {
    return SSE_E_GENERIC;
  }
  moat_uuid_generate(&uuid);
  moat_uuid_to_string(&uuid, uuid_str);
  err = moat_object_add_object_value(self->fSensingData, uuid_str, data, sse_false, sse_true);
  if (err) {
    moat_object_free(data);
    return err;
  }
  return SSE_E_OK;
}

static void
TDataCollector_HandleCommand(TDataCollector *self)
{
  sse_char *cmd = self->fBuffer;
  sse_int err;

  if (sse_strlen(cmd) > DC_VER_CMD_HEADER_LEN && sse_strncmp(cmd, ":VER:", DC_VER_CMD_HEADER_LEN) == 0) {
    cmd += DC_VER_CMD_HEADER_LEN;
    DC_LOG_INFO("Library Version:[%s]", cmd);
  } else if (sse_strlen(cmd) > DC_ERR_CMD_HEADER_LEN && sse_strncmp(cmd, ":ERR:", DC_ERR_CMD_HEADER_LEN) == 0) {
    cmd += DC_ERR_CMD_HEADER_LEN;
    DC_LOG_ERROR("Sensor Error:[%s]", cmd);
  } else if (sse_strlen(cmd) > DC_DATA_CMD_HEADER_LEN && sse_strncmp(cmd, ":DATA:", DC_DATA_CMD_HEADER_LEN) == 0) {
    cmd += DC_DATA_CMD_HEADER_LEN;
    err = TDataCollector_AddSensingData(self, cmd);
    DC_LOG_DEBUG("Sensor Data:[%s]", cmd);
  } else {
    DC_LOG_DEBUG("Command:[%s]", cmd);
  }
}

static void
DataCollector_HandleDataProc(MDFIO *in_io, sse_int in_flags, sse_pointer in_user_data)
{
  TDataCollector *dc = (TDataCollector *)in_user_data;
  sse_int read_bytes;
  sse_char *p;

  DC_ENTER();
  read_bytes = mdf_io_read(in_io, dc->fBuffer + dc->fReadBytes, sizeof(dc->fBuffer) - dc->fReadBytes);
  if (read_bytes == 0) {
    DC_LOG_DEBUG("disconnected.");
    mdf_io_stop(in_io);
    mdf_io_close(in_io);
    return;
  }
  if (read_bytes <= 0) {
    DC_LOG_ERROR("failed to read. err=[%d]", read_bytes);
    return;
  }
  if (dc->fReadBytes == 0) {
    if (dc->fBuffer[0] != ':') {
      dc->fReadBytes = 0;
      return;
    }
  }
  dc->fReadBytes += read_bytes;
  p = sse_strchr(dc->fBuffer, '\n');
  if (p != NULL) {
    *p = '\0';
    TDataCollector_HandleCommand(dc);
    dc->fReadBytes = 0;
    sse_memset(dc->fBuffer, 0, sizeof(dc->fBuffer));
  }
  DC_LEAVE();
}

static void
DataCollector_DeviceStatusChangedProc(MDFDevice *in_device, sse_int in_new_status, sse_pointer in_user_data)
{
  TDataCollector *dc = (TDataCollector *)in_user_data;
  const sse_char *dev_path = NULL;
  const sse_char *sn_v;
  sse_char *sn = NULL;
  MDFSerialPort *serial_port = NULL;
  MDFSerialAttributes attr;
  sse_int fd;
  sse_int64 interval;
  sse_int timer_id;
  sse_int err;

  DC_ENTER();
  dev_path = mdf_device_get_name(in_device);
  sn_v = mdf_device_get_value(in_device, "ID_SERIAL");
  switch (in_new_status) {
  case MDF_DEVICE_STATUS_ADD:
    if (dc->fSerialPort != NULL) {
      DC_LOG_ERROR("already attached.");
      goto error_exit;
    }
    if (dev_path == NULL) {
      DC_LOG_ERROR("dev name is nil.");
      goto error_exit;
    }
    if (sn_v != NULL) {
      sn = sse_strdup(sn_v);
    }
    serial_port = mdf_serial_port_new((sse_char *)dev_path);
    if (serial_port == NULL) {
      DC_LOG_ERROR("failed to create serial port.");
      goto error_exit;
    }
    fd = mdf_io_open((MDFIO *)serial_port, O_RDONLY | O_NOCTTY, DataCollector_HandleDataProc, dc, MOAT_IO_FLAG_READ);
    if (fd < 0) {
      DC_LOG_ERROR("failed to open device.");
      goto error_exit;
    }
    err = mdf_serial_port_get_attributes(serial_port, &attr);
    if (err) {
      DC_LOG_ERROR("failed to get attributes.");
      goto error_exit;
    }
    attr.BaudRate = DC_DEFAULT_BAUDRATE;
    err = mdf_serial_port_set_attributes(serial_port, &attr);
    if (err) {
      DC_LOG_ERROR("failed to set attributes.");
      goto error_exit;
    }
    err = mdf_io_start((MDFIO *)serial_port);
    if (err) {
      DC_LOG_ERROR("failed to start io.");
      goto error_exit;
    }
    err = moat_object_get_int64_value(dc->fConf, "dataUploadIntervalSec", &interval);
    if (err) {
      DC_LOG_ERROR("failed to get conf.");
      goto error_exit;
    }
    timer_id = moat_timer_set(dc->fTimer, interval, DataCollector_UploadIntervalProc, dc);
    if (timer_id < 0) {
      DC_LOG_ERROR("failed to set upload timer.");
      err = timer_id;
      goto error_exit;
    }
    dc->fSerialPort = serial_port;
    dc->fSerialNumber = sn;
    dc->fUploadTimerId = timer_id;
    break;
  case MDF_DEVICE_STATUS_REMOVE:
    moat_timer_cancel(dc->fTimer, dc->fUploadTimerId);
    if (dc->fSerialPort == NULL) {
      return;
    }
    if (mdf_io_is_started((MDFIO *)dc->fSerialPort)) {
      mdf_io_stop((MDFIO *)dc->fSerialPort);
    }
    if (mdf_io_is_opened((MDFIO *)dc->fSerialPort)) {
      mdf_io_close((MDFIO *)dc->fSerialPort);
    }
    mdf_serial_port_free(dc->fSerialPort);
    dc->fSerialPort = NULL;
    break;
  default:
    DC_LOG_ERROR("unknown action");
    break;
  }
  DC_LEAVE();
  return;

error_exit:
  if (serial_port != NULL) {
    mdf_serial_port_free(serial_port);
  }
  if (sn != NULL) {
    sse_free(sn);
  }
}

sse_int
TDataCollector_Start(TDataCollector *self)
{
  ModelMapper m;
  sse_char *type;
  sse_char *filter;
  sse_uint len;
  sse_int err;

  sse_memset(&m, 0, sizeof(m));
  err = moat_register_model(self->fMoat, "SensingData", &m, self);
  if (err) {
    goto error_exit;
  }
  err = moat_object_get_string_value(self->fConf, "deviceType", &type, &len);
  if (err) {
    goto error_exit;
  }
  err = moat_object_get_string_value(self->fConf, "deviceFilter", &filter, &len);
  if (err) {
    goto error_exit;
  }
  err = mdf_device_finder_start(self->fDeviceFinder, type, filter);
  if (err) {
    goto error_exit;
  }
  return SSE_E_OK;

error_exit:
  return err;
}

void
TDataCollector_Stop(TDataCollector *self)
{
  mdf_device_finder_stop(self->fDeviceFinder);
  moat_unregister_model(self->fMoat, "SensingData");
}

sse_bool
TDataCollector_IsStarted(TDataCollector *self)
{
  return mdf_device_finder_is_started(self->fDeviceFinder);
}

TDataCollector *
DataCollector_New(Moat in_moat, sse_char *in_urn, MoatObject *in_conf)
{
  TDataCollector *dc = NULL;
  MDFDeviceFinder *finder = NULL;
  MoatObject *obj = NULL;
  sse_char *noti_id = NULL;
  MoatTimer *timer = NULL;

  DC_ENTER();
  dc = sse_malloc(sizeof(TDataCollector));
  if (dc == NULL) {
    goto error_exit;
  }
  finder = mdf_device_finder_new(DataCollector_DeviceStatusChangedProc, dc);
  if (finder == NULL) {
    goto error_exit;
  }
  noti_id = moat_create_notification_id(in_urn, "upload-sensing-data", "1.0");
  if (noti_id == NULL) {
    goto error_exit;
  }
  obj = moat_object_new();
  if (obj == NULL) {
    goto error_exit;
  }
  timer = moat_timer_new();
  if (timer == NULL) {
    goto error_exit;
  }
  dc->fMoat = in_moat;
  dc->fConf = in_conf;
  dc->fDeviceFinder = finder;
  dc->fUploadSensingDataId = noti_id;
  dc->fReadBytes = 0;
  sse_memset(dc->fBuffer, 0, sizeof(dc->fBuffer));
  dc->fSerialPort = NULL;
  dc->fSensingData = obj;
  dc->fTimer = timer;
  DC_LEAVE();
  return dc;

error_exit:
  if (timer != NULL) {
    moat_timer_free(timer);
  }
  if (obj != NULL) {
    moat_object_free(obj);
  }
  if (noti_id != NULL) {
    sse_free(noti_id);
  }
  if (finder != NULL) {
    mdf_device_finder_free(finder);
  }
  if (dc != NULL) {
    sse_free(dc);
  }
  return NULL;
}

void
TDataCollector_Delete(TDataCollector *self)
{
  moat_timer_free(self->fTimer);
  moat_object_free(self->fSensingData);
  sse_free(self->fUploadSensingDataId);
  mdf_device_finder_free(self->fDeviceFinder);
  sse_free(self);
}
