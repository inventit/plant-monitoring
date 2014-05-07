#include <servicesync/moat.h>
#include "data_collector.h"
#include "image_collector.h"

static MoatObject *
create_default_conf(void)
{
  MoatObject *conf = NULL;
  sse_int err;

  conf = moat_object_new();
  if (conf == NULL) {
    return NULL;
  }
  err = moat_object_add_int64_value(conf, "dataUploadIntervalSec", 10, sse_true);
  if (err) {
    goto error_exit;
  }
  err = moat_object_add_int64_value(conf, "imageUploadIntervalSec", 3600, sse_true);
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
  sse_char *pkg_urn;
  Moat moat = NULL;
  sse_int err = SSE_E_OK;
  MoatObject *conf = NULL;
  TDataCollector *dc = NULL;
  TImageCollector *ic = NULL;

  pkg_urn = argv[0];
  err = moat_init(pkg_urn, &moat);
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
  dc = DataCollector_New(moat, pkg_urn, conf);
  if (dc == NULL) {
    goto error_exit;
  }
  ic = ImageCollector_New(moat, pkg_urn, conf);
  if (ic == NULL) {
    goto error_exit;
  }
  err = TDataCollector_Start(dc);
  if (err) {
    goto error_exit;
  }
  err = TImageCollector_Start(ic);
  if (err) {
    goto error_exit;
  }
  err = moat_run(moat);
  if (err != SSE_E_OK) {
    goto error_exit;
  }
  TImageCollector_Stop(ic);
  TDataCollector_Stop(dc);
  TImageCollector_Delete(ic);
  TDataCollector_Delete(dc);
  moat_object_free(conf);
  moat_destroy(moat);
  return SSE_E_OK;

error_exit:

  if (ic != NULL) {
    if (TImageCollector_IsStarted(ic)) {
      TImageCollector_Stop(ic);
    }
    TImageCollector_Delete(ic);
  }
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
