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

#ifndef MDF_DEVICE_FINDER_H_
#define MDF_DEVICE_FINDER_H_

SSE_BEGIN_C_DECLS

enum _device_status {
  MDF_DEVICE_STATUS_NONE = 0,
  MDF_DEVICE_STATUS_ADD,
  MDF_DEVICE_STATUS_REMOVE,
  MDF_DEVICE_STATUSs
};

typedef struct MDFDeviceFinder_ MDFDeviceFinder;
typedef void (*MDFDeviceFinder_DeviceStatusChangedProc)(MDFDevice *in_device, sse_int in_new_status, sse_pointer in_user_data);

MDFDeviceFinder * mdf_device_finder_new(MDFDeviceFinder_DeviceStatusChangedProc in_proc, sse_pointer in_user_data);
void mdf_device_finder_free(MDFDeviceFinder *self);
sse_int mdf_device_finder_start(MDFDeviceFinder *self, sse_char *in_type, sse_char *in_filter);
void mdf_device_finder_stop(MDFDeviceFinder *self);
sse_bool mdf_device_finder_is_started(MDFDeviceFinder *self);
void mdf_device_finder_set_statuc_changed_proc(MDFDeviceFinder *self, MDFDeviceFinder_DeviceStatusChangedProc in_proc, sse_pointer in_user_data);

SSE_END_C_DECLS

#endif /* MDF_DEVICE_FINDER_H_ */
