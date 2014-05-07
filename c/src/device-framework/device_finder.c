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

#include <libudev.h>
#include <servicesync/moat.h>
#include "device.h"
#include "device_finder.h"

#define MDFDF_LOG_ERROR(format, ...)   MOAT_LOG_ERROR("MDFDF", format, ##__VA_ARGS__)
#define MDFDF_LOG_INFO(format, ...)    MOAT_LOG_INFO("MDFDF", format, ##__VA_ARGS__)
#define MDFDF_LOG_DEBUG(format, ...)   MOAT_LOG_DEBUG("MDFDF", format, ##__VA_ARGS__)
#define MDFDF_LOG_TRACE(format, ...)   MOAT_LOG_TRACE("MDFDF", format, ##__VA_ARGS__)
#define MDFDF_ENTER()      MDFDF_LOG_TRACE("enter")
#define MDFDF_LEAVE()      MDFDF_LOG_TRACE("leave")

struct MDFDeviceFinder_ {
  struct udev *UDev;
  struct udev_monitor *UDevMonitor;
  MoatIOWatcher *Monitor;
  sse_char *Subsystem;
  sse_char *DevType;
  SSESList *Filter;
  MDFDeviceFinder_DeviceStatusChangedProc StatusChangedProc;
  sse_pointer StatusChangedParam;
};

static sse_bool
mdf_device_finder_is_matched_device(MDFDeviceFinder *self, struct udev_device *in_device)
{
  sse_bool matched = sse_false;
  SSESList *l;
  sse_char *key;
  sse_char *expected_v;
  const char *actual_v;

  if (self->Filter == NULL) {
    return sse_true;
  }
  l = self->Filter;
  while (l != NULL) {
    key = sse_string_get_cstr((SSEString *)l->data);
    expected_v = sse_strchr(key, '=');
    if (expected_v != NULL) {
      *expected_v = '\0';
      expected_v++;
    }
    actual_v = udev_device_get_property_value(in_device, key);
    if (actual_v == NULL) {
      break;
    }
    if (expected_v != NULL) {
      if (sse_strcmp(expected_v, actual_v) != 0) {
        break;
      }
    }
    l = l->next;
    if (l == NULL) {
      matched = sse_true;
    }
  }
  return matched;
}

static void
mdf_device_notify_status_changed(MDFDeviceFinder *self, struct udev_device *in_dev, sse_int in_new_status)
{
  MDFDF_ENTER();
  if (self->StatusChangedProc == NULL) {
    return;
  }
  (*self->StatusChangedProc)((MDFDevice *)in_dev, in_new_status, self->StatusChangedParam);
  MDFDF_LEAVE();
}

static SSESList *
mdf_device_finder_find_devices(MDFDeviceFinder *self)
{
  struct udev_enumerate *udev_enum;
  struct udev_list_entry *devices;
  struct udev_list_entry *device_entry;
  struct udev_device *device;
  const char *path;
  sse_int err;
  SSESList *result = NULL;
  sse_bool matched;

  MDFDF_ENTER();
  udev_enum = udev_enumerate_new(self->UDev);
  if (self->Subsystem != NULL) {
    err = udev_enumerate_add_match_subsystem(udev_enum, self->Subsystem);
    if (err) {
      MDFDF_LOG_ERROR("failed to udev_enumerate_add_match_subsystem()");
      return NULL;
    }
  }
  err = udev_enumerate_scan_devices(udev_enum);
  if (err) {
    MDFDF_LOG_ERROR("failed to udev_monitor_filter_add_match_subsystem_devtype()");
    return NULL;
  }
  devices = udev_enumerate_get_list_entry(udev_enum);
  udev_list_entry_foreach(device_entry, devices) {
    path = udev_list_entry_get_name(device_entry);
    device = udev_device_new_from_syspath(self->UDev, path);
    if (device == NULL) {
      continue;
    }
    matched = mdf_device_finder_is_matched_device(self, device);
    if (matched) {
      result = sse_slist_add(result, device);
    } else {
      udev_device_unref(device);
    }
  }
  MDFDF_LEAVE();
  return result;
}

static void
mdf_device_finder_udev_event_proc(MoatIOWatcher *in_watcher, sse_pointer in_user_data, sse_int in_desc, sse_int in_event_flags)
{
  MDFDeviceFinder *finder = (MDFDeviceFinder *)in_user_data;
  struct udev_device *device = NULL;
  const char *action;
  sse_bool matched;

  MDFDF_ENTER();
  device = udev_monitor_receive_device(finder->UDevMonitor);
  if (device == NULL ) {
    MDFDF_LOG_ERROR("device is nil");
    return;
  }
  matched = mdf_device_finder_is_matched_device(finder, device);
  if (!matched) {
    udev_device_unref(device);
    MDFDF_LOG_ERROR("not matched");
    return;
  }
  action = udev_device_get_action(device);
  if (action == NULL) {
    MDFDF_LOG_ERROR("action is nil");
  } else if (sse_strcmp(action, "add") == 0) {
    MDFDF_LOG_DEBUG("device was attactched");
    mdf_device_notify_status_changed(finder, device, MDF_DEVICE_STATUS_ADD);
  } else if (sse_strcmp(action, "remove") == 0) {
    MDFDF_LOG_DEBUG("device was detached");
    mdf_device_notify_status_changed(finder, device, MDF_DEVICE_STATUS_REMOVE);
  } else {
    MDFDF_LOG_DEBUG("unknown action:[%s]", action);
  }
  udev_device_unref(device);
  MDFDF_LEAVE();
}

sse_int
mdf_device_finder_start(MDFDeviceFinder *self, sse_char *in_type, sse_char *in_filter)
{
  sse_char *subsystem;
  sse_char *type;
  SSEString *filter;
  sse_char *p;
  SSESList *devices;
  struct udev_device *device;
  sse_int err;

  MDFDF_ENTER();
  if (in_type != NULL) {
    sse_char *dup = NULL;
    dup = sse_strdup(in_type);
    if (dup == NULL) {
      err = SSE_E_NOMEM;
      goto error_exit;
    }
    subsystem = dup;
    type = NULL;
    p = sse_strchr(subsystem, ':');
    if (p != NULL) {
      *p = '\0';
      type = (p + 1);
    }
    MDFDF_LOG_DEBUG("subsystem=[%s], devtype=[%s]", subsystem, type);
    err = udev_monitor_filter_add_match_subsystem_devtype(self->UDevMonitor, subsystem, type);
    if (err) {
      sse_free(dup);
      goto error_exit;
    }
    self->Subsystem = subsystem;
    self->DevType = type;
  }
  if (in_filter != NULL) {
    MDFDF_LOG_DEBUG("filter=[%s]", in_filter);
    filter = sse_string_new(in_filter);
    self->Filter = sse_string_split(filter, ",", -1);
  }
  devices = mdf_device_finder_find_devices(self);
  while (devices != NULL) {
    device = (struct udev_device *)devices->data;
    mdf_device_notify_status_changed(self, device, MDF_DEVICE_STATUS_ADD);
    devices = sse_slist_remove(devices, device);
    udev_device_unref(device);
  }
  moat_io_watcher_start(self->Monitor);
  MDFDF_LEAVE();
error_exit:
  return err;
}

void
mdf_device_finder_stop(MDFDeviceFinder *self)
{
  if (moat_io_watcher_is_active(self->Monitor)) {
    moat_io_watcher_stop(self->Monitor);
  }
}

sse_bool
mdf_device_finder_is_started(MDFDeviceFinder *self)
{
  return moat_io_watcher_is_active(self->Monitor);
}

void
mdf_device_finder_set_statuc_changed_proc(MDFDeviceFinder *self, MDFDeviceFinder_DeviceStatusChangedProc in_proc, sse_pointer in_user_data)
{
  self->StatusChangedProc = in_proc;
  self->StatusChangedParam = in_user_data;
}

MDFDeviceFinder *
mdf_device_finder_new(MDFDeviceFinder_DeviceStatusChangedProc in_proc, sse_pointer in_user_data)
{
  MDFDeviceFinder *finder = NULL;
  struct udev *udev = NULL;
  struct udev_monitor *udev_monitor = NULL;
  MoatIOWatcher *w = NULL;
  sse_int fd;
  sse_int err;

  finder = sse_zeroalloc(sizeof(MDFDeviceFinder));
  if (finder == NULL) {
    return NULL;
  }
  udev = udev_new();
  if (udev == NULL) {
    goto error_exit;
  }
  udev_monitor = udev_monitor_new_from_netlink(udev, "udev");
  if (udev_monitor == NULL) {
    goto error_exit;
  }
  err = udev_monitor_enable_receiving(udev_monitor);
  if (err) {
    goto error_exit;
  }
  fd = udev_monitor_get_fd(udev_monitor);
  if (fd < 0) {
    goto error_exit;
  }
  w = moat_io_watcher_new(fd, mdf_device_finder_udev_event_proc, finder, MOAT_IO_FLAG_READ);
  if (w == NULL) {
    goto error_exit;
  }
  finder->UDev = udev;
  finder->UDevMonitor = udev_monitor;
  finder->Monitor = w;
  finder->StatusChangedProc = in_proc;
  finder->StatusChangedParam = in_user_data;
  return finder;

error_exit:
  if (w != NULL) {
    moat_io_watcher_free(w);
  }
  if (udev_monitor != NULL) {
    udev_monitor_unref(udev_monitor);
  }
  if (udev != NULL) {
    udev_unref(udev);
  }
  if (finder != NULL) {
    sse_free(finder);
  }
  return NULL;
}

void
mdf_device_finder_free(MDFDeviceFinder *self)
{
  mdf_device_finder_stop(self);
  moat_io_watcher_free(self->Monitor);
  udev_monitor_unref(self->UDevMonitor);
  udev_unref(self->UDev);
  sse_free(self);
}
