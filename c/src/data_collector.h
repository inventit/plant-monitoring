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

#ifndef DATA_COLLECTOR_H_
#define DATA_COLLECTOR_H_


SSE_BEGIN_C_DECLS

typedef struct TDataCollector_ TDataCollector;

TDataCollector * DataCollector_New(Moat in_moat, MoatObject *in_conf);
void TDataCollector_Delete(TDataCollector *self);
sse_int TDataCollector_Start(TDataCollector *self);
void TDataCollector_Stop(TDataCollector *self);
sse_bool TDataCollector_IsStarted(TDataCollector *self);

SSE_END_C_DECLS

#endif /* DATA_COLLECTOR_H_ */
