// Sensors:
//   Temperature, Humidity : Aosong AM2302 (www.aosong.com)
//   Moisture : DFROBOT SKU:SEN0114 (http://www.dfrobot.com/wiki/index.php?title=Moisture_Sensor_(SKU:SEN0114))
// Library for AM2303 (DHT22)
//    http://playground.arduino.cc/Main/DHTLib
#include <dht.h>

#define BAUDRATE 57600
#define SENSING_INTERVAL_MS 1000
#define AM2302_PIN 2
#define SEN0114_PIN 0

void printVersion(void)
{
  Serial.print(":VER:");
  Serial.println(DHT_LIB_VERSION);
}

void printError(int err)
{
  Serial.print(":ERR:");
  switch (err) {
  case DHTLIB_ERROR_CHECKSUM: 
    Serial.println("CHECKSUM"); 
    break;
  case DHTLIB_ERROR_TIMEOUT: 
    Serial.println("TIMEOUT"); 
    break;
  default: 
    Serial.println("UNKNOWN"); 
    break;
  }
}

void printAllData(double temperature, double humidity, int moisture)
{
  Serial.print(":DATA:");
  Serial.print("{\"temperature\":");
  Serial.print(temperature, 1);
  Serial.print(",\"humidity\":");
  Serial.print(humidity, 1);
  Serial.print(",\"moisture\":");
  Serial.print(moisture, 1);
  Serial.println("}");
}

void printMoisture(int moisture)
{
  Serial.print(":DATA:");
  Serial.print("{\"moisture\":");
  Serial.print(moisture, 1);
  Serial.println("}");
}

void setup()
{
  Serial.begin(BAUDRATE);
  Serial.print(":VER:");
  Serial.println(DHT_LIB_VERSION);
}

void loop()
{
  dht DHT;
  int moisture;
  int result;

  moisture = analogRead(SEN0114_PIN);
  result = DHT.read22(AM2302_PIN);
  if (result == DHTLIB_OK) {
    printAllData(DHT.temperature, DHT.humidity, moisture);
  } else {
    printError(result);
    printMoisture(moisture);
  }
  delay(SENSING_INTERVAL_MS);
}

