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

#ifndef MDFFINDER_H_
#define MDFFINDER_H_

SSE_BEGIN_C_DECLS

enum _device_status {
  MDF_DEVICE_STATUS_NONE = 0,
  MDF_DEVICE_STATUS_ADD,
  MDF_DEVICE_STATUS_REMOVE,
  MDF_DEVICE_STATUSs
};

typedef struct MDFFinder_ MDFFinder;
typedef void (*MDFFinder_DeviceStatusChangedProc)(MDFDevice *in_device, sse_int in_new_status, sse_pointer in_user_data);

MDFFinder * mdf_finder_new(MDFFinder_DeviceStatusChangedProc in_proc, sse_pointer in_user_data);
void mdf_finder_free(MDFFinder *self);
sse_int mdf_finder_start(MDFFinder *self, sse_char *in_type, sse_char *in_filter);
void mdf_finder_stop(MDFFinder *self);
sse_bool mdf_finder_is_started(MDFFinder *self);
void mdf_finder_set_statuc_changed_proc(MDFFinder *self, MDFFinder_DeviceStatusChangedProc in_proc, sse_pointer in_user_data);

SSE_END_C_DECLS

#endif /* MDFFINDER_H_ */
