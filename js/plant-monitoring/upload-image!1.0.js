/*
 * JobServiceID:
 * urn:moat:${APPID}:plant-monitoring:upload-image:1.0
 * 
 * Description: Notify captured image to the Web UI application.
 */
var moat = require('moat');
var context = moat.init();
var session = context.session;
var clientRequest = context.clientRequest;
var logTag = 'upload-image'

var WEBAPI_IMAGE = 'https://plant-monitoring-test.herokuapp.com/contents/notify_image/';
//var WEBAPI_IMAGE = 'http://localhost:3000/contents/notify_image/';

session.log(logTag, '--- Image arrived ---');

var objects = clientRequest.objects;
if (objects.length != 1) {
  throw '*** ERROR!! *** invalid object length:' + objects.length;
}
if (objects[0].array.length != 1) {
  throw '*** ERROR!! *** invalid array length:' + objects[0].array.length;
}
notifyImage(clientRequest.device.deviceId, clientRequest.objects[0].array[0]);

function notifyImage(deviceId, entity) {
  session.fetchUrlSync(
    WEBAPI_IMAGE,
    {
      method : 'POST',
      contentType : 'application/json',
      payload : {
        deviceId : deviceId,
        uid : entity.uid,
        timestamp : entity.timestamp,
        contentType : entity.contentType,
        encodedContent : entity.encodedContent
      }
    },
    function(response) {
      if (parseInt(response.responseCode / 100) == 2) {
        session.log(logTag, 'Notified');
      } else {
        session.log(logTag, '*** ERROR!! *** resp code: ' + response.responseCode);
        throw '*** ERROR!! *** resp code: ' + response.responseCode;
      }
    }
  );
}

