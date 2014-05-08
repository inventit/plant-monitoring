/*
 * JobServiceID:
 * urn:moat:${APPID}:plant-monitoring:upload-sensing-data:1.0
 * 
 * Description: Notify captured image to the Web UI application.
 */
var moat = require('moat');
var context = moat.init();
var session = context.session;
var clientRequest = context.clientRequest;
var logTag = 'upload-sensing-data'

var WEBAPI_SENSING_DATA = 'https://plant-monitoring-test.herokuapp.com/contents/notify_sensing_data/';
//var WEBAPI_SENSING_DATA = 'http://localhost:3000/contents/notify_sensing_data/';

session.log(logTag, '--- Data arrived ---');

var objects = clientRequest.objects;
if (objects.length != 1) {
  throw '*** ERROR!! *** invalid object length:' + objects.length;
}

var array = rawDataToNotifyData(objects[0].array);
notifyData(clientRequest.device.deviceId, array);

function rawDataToNotifyData(rawArray) {
  var array = [];
  for (var i = 0; i < rawArray.length; i++) {
    var e = rawArray[i];
    var entity = {
      uid : e.uid,
      serialNumber : e.serialNumber,
      temperature : parseFloat(e.temperature),
      humidity : parseFloat(e.humidity),
      moisture : e.moisture,
      timestamp : e.timestamp
    };
    array.push(entity);
  }
  return array;
}
function notifyData(deviceId, rawData) {
  session.fetchUrlSync(
    WEBAPI_SENSING_DATA,
    {
      method : 'POST',
      contentType : 'application/json',
      payload : {
        deviceId : deviceId,
        array: array
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

