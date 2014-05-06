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

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <servicesync/moat.h>
#include "io.h"

static void
mdf_io_event_proc(MoatIOWatcher *in_watcher, sse_pointer in_user_data, sse_int in_desc, sse_int in_event_flags)
{
  MDFIO *io = (MDFIO *)in_user_data;
  if (in_event_flags & io->EventFlags) {
    if (io->EventHandler != NULL) {
      (*io->EventHandler)(io, in_event_flags, io->UserData);
    }
  }
}

sse_int
mdf_io_start(MDFIO *self)
{
  if (self->Channel == NULL) {
    return SSE_E_INVAL;
  }
  return moat_io_watcher_start(self->Channel);
}

void
mdf_io_stop(MDFIO *self)
{
  if (self->Channel != NULL) {
    if (moat_io_watcher_is_active(self->Channel)) {
      moat_io_watcher_stop(self->Channel);
    }
  }
}

sse_bool
mdf_io_is_started(MDFIO *self)
{
  if (self->Channel == NULL) {
    return sse_false;
  }
  return moat_io_watcher_is_active(self->Channel);
}

sse_int
mdf_io_open(MDFIO *self, sse_int in_flags, MDFIO_EventProc in_proc, sse_pointer in_user_data, sse_int in_event_flags)
{
  int fd;
  MoatIOWatcher *ch = NULL;

  if (self->Path == NULL) {
    return SSE_E_INVAL;
  }
  if (self->OpenProc != NULL) {
    fd = (*self->OpenProc)(self, in_flags);
  } else {
    fd = open(self->Path, in_flags);
  }
  if (fd < 0) {
    return fd;
  }
  ch = moat_io_watcher_new(fd, mdf_io_event_proc, self, in_event_flags);
  self->Desc = fd;
  self->Channel = ch;
  self->EventHandler = in_proc;
  self->UserData = in_user_data;
  self->EventFlags = in_event_flags;
  return fd;

}

void
mdf_io_close(MDFIO *self)
{
  if (self->CloseProc != NULL) {
    (*self->CloseProc)(self);
  } else {
    close(self->Desc);
  }
  self->Desc = -1;
}

sse_ssize
mdf_io_read(MDFIO *self, void *out_buffer, sse_size in_buf_size)
{
  if (self->ReadProc != NULL) {
    return (*self->ReadProc)(self, out_buffer, in_buf_size);
  }
  return read(self->Desc, out_buffer, in_buf_size);
}

sse_ssize
mdf_io_write(MDFIO *self, void *in_buffer, sse_size in_buf_size)
{
  if (self->WriteProc != NULL) {
    return (*self->WriteProc)(self, in_buffer, in_buf_size);
  }
  return write(self->Desc, in_buffer, in_buf_size);
}



sse_int
mdf_io_init(MDFIO *in_io)
{
  in_io->Desc = -1;
  in_io->Channel = NULL;
  in_io->EventFlags = 0;
  in_io->Path = NULL;
  return SSE_E_OK;
}

void
mdf_io_fini(MDFIO *in_io)
{
  if (mdf_io_is_started(in_io)) {
    mdf_io_stop(in_io);
  }
  if (in_io->Channel != NULL) {
    moat_io_watcher_free(in_io->Channel);
    in_io->Channel = NULL;
  }
  if (in_io->Desc >= 0) {
    mdf_io_close(in_io);
    in_io->Desc = -1;
  }
  in_io->EventFlags = 0;
}
