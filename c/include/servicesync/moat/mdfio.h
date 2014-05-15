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

#ifndef MDFIO_H_
#define MDFIO_H_


SSE_BEGIN_C_DECLS

typedef struct MDFIO_ MDFIO;
typedef void (*MDFIO_EventProc)(MDFIO *in_io, sse_int in_flags, sse_pointer in_user_data);

struct MDFIO_ {
  sse_int (*OpenProc)(MDFIO *self, sse_int in_flags);
  sse_int (*CloseProc)(MDFIO *self);
  sse_ssize (*ReadProc)(MDFIO *self, void *out_buffer, sse_size in_buf_size);
  sse_ssize (*WriteProc)(MDFIO *self, void *in_buffer, sse_size in_buf_size);;
  sse_char *Path;
  sse_int Desc;
  MoatIOWatcher *Channel;
  sse_int EventFlags;
  MDFIO_EventProc EventHandler;
  sse_pointer UserData;
};

sse_int mdf_io_init(MDFIO *in_io);
void mdf_io_fini(MDFIO *in_io);
sse_int mdf_io_open(MDFIO *self, sse_int in_flags, MDFIO_EventProc in_proc, sse_pointer in_user_data, sse_int in_event_flags);
void mdf_io_close(MDFIO *self);
sse_bool mdf_io_is_opened(MDFIO *self);
sse_int mdf_io_start(MDFIO *self);
void mdf_io_stop(MDFIO *self);
sse_bool mdf_io_is_started(MDFIO *self);
sse_ssize mdf_io_read(MDFIO *self, void *out_buffer, sse_size in_buf_size);
sse_ssize mdf_io_write(MDFIO *self, void *in_buffer, sse_size in_buf_size);

SSE_END_C_DECLS

#endif /* MDFIO_H_ */
