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
 * 9F KOJIMACHI CP BUILDING
 * 4-4-7 Kojimachi, Chiyoda-ku, Tokyo 102-0083
 * JAPAN
 * http://www.yourinventit.com/
 */

#include <libudev.h>
#include <servicesync/moat.h>
#include "device.h"

typedef struct udev_device MDFDevice_;

sse_char *
mdf_device_get_name(MDFDevice *self)
{
  struct udev_device *dev = (struct udev_device *)self;
  return (sse_char *)udev_device_get_devnode(dev);
}

sse_char *
mdf_device_get_value(MDFDevice *self, sse_char *in_key)
{
  struct udev_device *dev = (struct udev_device *)self;
  return (sse_char *)udev_device_get_property_value(dev, in_key);
}
