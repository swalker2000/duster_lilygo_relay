# Example of switch firmware based on esp32 (lilygo T-Relay) operating with a delivery guarantee service running over MQTT
## https://github.com/swalker2000/duster_broker
(relay numbering starts from 1)<br><br>
Example consumer based on esp32: https://github.com/swalker2000/duster_esp32_example
To compile, you need to create a Secret.h file in the sketch's main directory with the following contents:
```c++
#define SSID          "MY_SSID"
#define WIFI_PASS     "MY_WIFI_PASS"
#define URL           "MQTT_URL"
#define PORT          8883
#define MQTT_USERNAME "MQTT_USERNAME"
#define MQTT_PASS     "MQTT_PASSWORD"
```

## RelayWrite (closing and opening a relay with a given number)

Example command sent to the broker to close relay 2. <br>
Topic: producer/request/device123

```json
  {
        "believerGuarantee":"RECEIPT_CONFIRMATION",
        "command":"relayWrite",
        "data": {
            "relayNumber": 2,
            "pinValue" : true
        }
  }
```

Example command received by the switch to close relay 2. <br>
Topic: (consumer/request/device123)

```json
  {
        "id":2,
        "believerGuarantee":"RECEIPT_CONFIRMATION",
        "command":"relayWrite",
        "currentTimestamp":1772021717684,
        "data": {
            "relayNumber": 2,
            "pinValue" : true
        }
   }
```

## Blink (closing and opening a relay with a given periodicity, a given number of times)

Example command sent to the broker to make relay 2 close and open 4 times with a periodicity of 2,000 milliseconds. <br>
Topic: producer/request/device123

```json
  {
        "believerGuarantee":"RECEIPT_CONFIRMATION",
        "command":"blink",
        "data": {
            "relayNumber": 2,
            "period" : 2000,
            "count" : 4
        }
  }
```

Example command received by the switch to make relay 2 close and open 4 times with a periodicity of 2,000 milliseconds. <br> 
Topic: (consumer/request/device123)

```json
  {
        "id":2,
        "believerGuarantee":"RECEIPT_CONFIRMATION",
        "command":"blink",
        "currentTimestamp":1772021717684,
        "data": {
            "relayNumber": 2,
            "period" : 2000,
            "count" : 4
        }
   }
```