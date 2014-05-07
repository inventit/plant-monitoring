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

#ifndef IMAGE_COLLECTOR_H_
#define IMAGE_COLLECTOR_H_


SSE_BEGIN_C_DECLS

typedef struct TImageCollector_ TImageCollector;

TImageCollector * ImageCollector_New(Moat in_moat, sse_char *in_pkg_urn, MoatObject *in_conf);
void TImageCollector_Delete(TImageCollector *self);
sse_int TImageCollector_Start(TImageCollector *self);
void TImageCollector_Stop(TImageCollector *self);
sse_bool TImageCollector_IsStarted(TImageCollector *self);

SSE_END_C_DECLS

#endif /* IMAGE_COLLECTOR_H_ */
