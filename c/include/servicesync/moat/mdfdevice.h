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

#ifndef MDFDEVICE_H_
#define MDFDEVICE_H_

SSE_BEGIN_C_DECLS

typedef struct MDFDevice_ MDFDevice;

sse_char * mdf_device_get_name(MDFDevice *self);
sse_char * mdf_device_get_value(MDFDevice *self, sse_char *in_key);

SSE_END_C_DECLS

#endif /* MDFDEVICE_H_ */
