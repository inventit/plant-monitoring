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

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <ev.h>
#include <servicesync/moat.h>
#include "image_collector.h"

#define IC_CONTENT_TYPE "image/jpeg"
#define IC_IMAGE_FILE   "image.jpg"

#define IC_LOG_ERROR(format, ...)   MOAT_LOG_ERROR("Collector", format, ##__VA_ARGS__)
#define IC_LOG_INFO(format, ...)    MOAT_LOG_INFO("Collector", format, ##__VA_ARGS__)
#define IC_LOG_DEBUG(format, ...)   MOAT_LOG_DEBUG("Collector", format, ##__VA_ARGS__)
#define IC_LOG_TRACE(format, ...)   MOAT_LOG_TRACE("Collector", format, ##__VA_ARGS__)
#define IC_ENTER()      IC_LOG_TRACE("enter")
#define IC_LEAVE()      IC_LOG_TRACE("leave")

struct TImageCollector_ {
  Moat fMoat;
  sse_char *fUploadImageId;
  MoatObject *fConf;
  MoatTimer *fTimer;
  sse_int fIntervalTimerId;
  ev_child fChildWatcher;
  sse_int fCurrentChildId;
};

static MoatObject *
ImageCollector_CreateImageData(void)
{
  MoatObject *data = NULL;
  struct stat sb;
  int fd = -1;
  sse_byte *image = NULL;
  sse_byte *p;
  sse_int remain;
  sse_int read_bytes;
  sse_size encoded_len;
  sse_char *b64 = NULL;
  sse_int64 ts;
  sse_int err;

  data = moat_object_new();
  if (data == NULL) {
    goto error_exit;
  }
  fd = open(IC_IMAGE_FILE, O_RDONLY);
  if (fd < 0) {
    goto error_exit;
  }
  err = fstat(fd, &sb);
  if (err < 0) {
    goto error_exit;
  }
  image = sse_malloc(sb.st_size);
  if (image == NULL) {
    goto error_exit;
  }
  remain = sb.st_size;
  p = image;
  while (1) {
    read_bytes = read(fd, (void *)p, remain);
    if (read_bytes < 0) {
      goto error_exit;
    }
    p += read_bytes;
    remain -= read_bytes;
    if (remain <= 0) {
      break;
    }
  }
  close(fd);
  fd = -1;
  encoded_len = sse_base64_get_encoded_length(sb.st_size);
  b64 = sse_malloc(encoded_len + 1);
  sse_base64_encode(image, sb.st_size, b64);
  b64[encoded_len] = '\0';
  sse_free(image);
  image = NULL;
  ts = moat_get_timestamp_msec();
  err = moat_object_add_string_value(data, "contentType", IC_CONTENT_TYPE, sse_strlen(IC_CONTENT_TYPE), sse_true, sse_true);
  if (err) {
    goto error_exit;
  }
  err = moat_object_add_string_value(data, "encodedContent", b64, encoded_len, sse_false, sse_true);
  if (err) {
    goto error_exit;
  }
  err = moat_object_add_int64_value(data, "timestamp", ts, sse_true);
  if (err) {
    goto error_exit;
  }
  return data;

error_exit:
  if (image != NULL) {
    sse_free(image);
  }
  if (fd >= 0) {
    close(fd);
  }
  if (data != NULL) {
    moat_object_free(data);
  }
  return NULL;
}

static sse_int
TImageCollector_UploadImage(TImageCollector *self)
{
  MoatObject *wrapper;
  MoatObject *image_data;
  MoatUUID uuid;
  sse_char uuid_str[MOAT_UUID_STRING_BUF_SIZE];
  sse_int err;
  sse_int req_id;

  IC_ENTER();
  image_data = ImageCollector_CreateImageData();
  if (image_data == NULL) {
    IC_LOG_ERROR("failed to create image data.");
    return SSE_E_GENERIC;
  }
  moat_uuid_generate(&uuid);
  moat_uuid_to_string(&uuid, uuid_str);
  wrapper = moat_object_new();
  err = moat_object_add_object_value(wrapper, uuid_str, image_data, sse_false, sse_true);
  if (err) {
    moat_object_free(image_data);
    return err;
  }
  req_id = moat_send_notification(self->fMoat, self->fUploadImageId, NULL, "CapturedImage", wrapper, NULL, NULL);
  moat_object_free(wrapper);
  if (req_id < 0) {
    IC_LOG_ERROR("failed to send notification");
    return SSE_E_GENERIC;
  }
  IC_LEAVE();
  return SSE_E_OK;
}

static void
TImageCollector_ChildCallback(struct ev_loop *loop, ev_child *w, int revents)
{
  TImageCollector *ic = (TImageCollector *)w->data;
  sse_int status;

  IC_ENTER();
  ev_child_stop(loop, w);
  IC_LOG_DEBUG("process %d exited with status %x", w->rpid, w->rstatus);
  waitpid(w->rpid, &status, WNOHANG);
  ic->fCurrentChildId = -1;
  TImageCollector_UploadImage(ic);
  IC_LEAVE();
}

static sse_int
TImageCollector_DoCapture(TImageCollector *self)
{
  pid_t child;
  struct ev_loop *loop;
  int err;

  if (self->fCurrentChildId >= 0) {
    IC_LOG_INFO("");
    return SSE_E_INVAL;
  }
  child = fork();
  if (child < 0) {
    return SSE_E_GENERIC;
  }
  if (child == 0) {
    err = system("raspistill -q 50 -w 200 -h 200 -o " IC_IMAGE_FILE);
    exit(err);
  }
  self->fCurrentChildId = child;
  ev_child_init(&self->fChildWatcher, TImageCollector_ChildCallback, child, 0);
  self->fChildWatcher.data = self;
  loop = ev_default_loop(0);
  ev_child_start(loop, &self->fChildWatcher);
  return SSE_E_OK;
}

static sse_bool
ImageCollector_IntervalProc(sse_int in_timer_id, sse_pointer in_user_data)
{
  TImageCollector *ic = (TImageCollector *)in_user_data;
  sse_int err;

  IC_ENTER();
  err = TImageCollector_DoCapture(ic);
  if (err) {
    IC_LOG_ERROR("failed to capture image.");
  }
  IC_LEAVE();
  return sse_true;
}

sse_int
TImageCollector_Start(TImageCollector *self)
{
  ModelMapper m;
  sse_int64 interval;
  sse_int timer_id;
  sse_int err;

  sse_memset(&m, 0, sizeof(m));
  err = moat_register_model(self->fMoat, "CapturedImage", &m, self);
  if (err) {
    goto error_exit;
  }
  err = moat_object_get_int64_value(self->fConf, "imageUploadIntervalSec", &interval);
  if (err) {
    goto error_exit;
  }
  TImageCollector_DoCapture(self);
  timer_id = moat_timer_set(self->fTimer, interval, ImageCollector_IntervalProc, self);
  if (timer_id < 0) {
    IC_LOG_ERROR("failed to set upload timer.");
    err = timer_id;
    goto error_exit;
  }
  self->fIntervalTimerId = timer_id;
  return SSE_E_OK;

error_exit:
  return err;
}

void
TImageCollector_Stop(TImageCollector *self)
{
  moat_timer_cancel(self->fTimer, self->fIntervalTimerId);
  self->fIntervalTimerId = -1;
  moat_unregister_model(self->fMoat, "CapturedImage");
}

sse_bool
TImageCollector_IsStarted(TImageCollector *self)
{
  return self->fIntervalTimerId >= 0;
}

TImageCollector *
ImageCollector_New(Moat in_moat, sse_char *in_urn, MoatObject *in_conf)
{
  TImageCollector *ic = NULL;
  sse_char *noti_id = NULL;
  MoatTimer *timer = NULL;

  IC_ENTER();
  ic = sse_zeroalloc(sizeof(TImageCollector));
  if (ic == NULL) {
    goto error_exit;
  }
  noti_id = moat_create_notification_id(in_urn, "upload-image", "1.0");
  if (noti_id == NULL) {
    goto error_exit;
  }
  timer = moat_timer_new();
  if (timer == NULL) {
    goto error_exit;
  }
  ic->fMoat = in_moat;
  ic->fConf = in_conf;
  ic->fUploadImageId = noti_id;
  ic->fTimer = timer;
  ic->fIntervalTimerId = -1;
  ic->fCurrentChildId = -1;
  IC_LEAVE();
  return ic;

error_exit:
  if (timer != NULL) {
    moat_timer_free(timer);
  }
  if (noti_id != NULL) {
    sse_free(noti_id);
  }
  if (ic != NULL) {
    sse_free(ic);
  }
  return NULL;
}

void
TImageCollector_Delete(TImageCollector *self)
{
  moat_timer_free(self->fTimer);
  sse_free(self->fUploadImageId);
  sse_free(self);
}
