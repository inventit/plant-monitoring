#include <servicesync/moat.h>
#include "data_collector.h"
#include "data_uploader.h"
#include "image_uploader.h"

static MoatObject *
create_default_conf(void)
{
  MoatObject *conf = NULL;
  sse_int err;

  conf = moat_object_new();
  if (conf == NULL) {
    return NULL;
  }
  err = moat_object_add_int32_value(conf, "dataUploadIntervalSec", 10, sse_true);
  if (err) {
    goto error_exit;
  }
  err = moat_object_add_int32_value(conf, "imageUploadIntervalSec", 3600, sse_true);
  if (err) {
    goto error_exit;
  }
  err = moat_object_add_string_value(conf, "deviceType", "tty", 3, sse_true, sse_true);
  if (err) {
    goto error_exit;
  }
  err = moat_object_add_string_value(conf, "deviceFilter", "ID_VENDOR_ID=2341,ID_MODEL_ID=0043", 34, sse_true, sse_true);
  if (err) {
    goto error_exit;
  }
  return conf;

error_exit:
  if (conf != NULL) {
    moat_object_free(conf);
  }
  return NULL;
}

sse_int
moat_app_main(sse_int in_argc, sse_char *argv[])
{
  Moat moat = NULL;
  sse_int err = SSE_E_OK;
  MoatObject *conf = NULL;
  TDataCollector *dc = NULL;
//  TImageUploader *iu = NULL;

  err = moat_init(argv[0], &moat);
  if (err != SSE_E_OK) {
    goto error_exit;
  }
  err = moat_json_file_to_moat_object("plant-monitoring.conf", &conf, NULL);
  if (err) {
    conf = create_default_conf();
    if (conf == NULL) {
      goto error_exit;
    }
  }
  dc = DataCollector_New(moat, conf);
  if (dc == NULL) {
    goto error_exit;
  }
#if 0
  iu = ImageUploader_New(moat, conf);
  if (iu == NULL) {
    goto error_exit;
  }
#endif
  err = TDataCollector_Start(dc);
  if (err) {
    goto error_exit;
  }
#if 0
  err = TImageUploader_Start(iu);
  if (err) {
    goto error_exit;
  }
#endif
  err = moat_run(moat);
  if (err != SSE_E_OK) {
    goto error_exit;
  }
//  TImageUploader_Stop(iu);
  TDataCollector_Stop(dc);
//  TImageUploader_Delete(iu);
  TDataCollector_Delete(dc);
  moat_object_free(conf);
  moat_destroy(moat);
  return SSE_E_OK;

error_exit:

#if 0
  if (iu != NULL) {
    if (TImageUploader_IsStarted(iu)) {
      TImageUploader_Stop(iu);
    }
    TImageUploader_Delete(iu);
  }
#endif
  if (dc != NULL) {
    if (TDataCollector_IsStarted(dc)) {
      TDataCollector_Stop(dc);
    }
    TDataCollector_Delete(dc);
  }
  if (conf != NULL) {
    moat_object_free(conf);
  }
  if (moat != NULL) {
    moat_destroy(moat);
  }
  return err;
}
