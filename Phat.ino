#include <VirtualWire.h>
#include "DHT.h"

#define DHTPIN 4  
#define DHTTYPE DHT11 
const int transmit_pin = 12;
struct package
{
  int temperature ;
  int humidity ;
};
typedef struct package Package;
Package data;

DHT dht(DHTPIN, DHTTYPE);
void setup()
{
    // Initialise the IO and ISR
    vw_set_tx_pin(transmit_pin);
    vw_set_ptt_inverted(true); // Required for DR3100
    vw_setup(500);       // Bits per sec
}
void loop()
{
  readSensor();
  vw_send((uint8_t *)&data, sizeof(data));
  vw_wait_tx(); // Wait until the whole message is gone
  delay(2000);
}
void readSensor()
{
  dht.begin();
  delay(1000);
  data.humidity = dht.readHumidity();
  data.temperature = dht.readTemperature();
}