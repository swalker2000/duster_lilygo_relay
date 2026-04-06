# Пример прошивки коммутатора на базе esp32 (lilygo T-Relay ) работающего с сервисом гарантии доставки работающим поверх MQTT
## https://github.com/swalker2000/duster_broker
(нумерация реле начинается с 1)<br><br>
Пример consumer на основе esp32 : https://github.com/swalker2000/duster_esp32_example
Для компиляции необходимо в основной директории скетча создать файл Secret.h со следующим содержимым: 
```c++
#define SECRET_WIFI_SSID "MY_SSID"
#define WIFI_PASS     "MY_WIFI_PASS"
#define URL           "MQTT_URL"
#define PORT          8883
#define MQTT_USERNAME "MQTT_USERNAME"
#define MQTT_PASS     "MQTT_PASSWORD"
```

## RelayWrite (замыкание и размыкани реле с заданным номером)

Пример команды отправляемой на брокер для того что бы реле 2 замкнулось. <br>
Топик : producer/request/device123

```json
  {
        "believerGuarantee":"RECEIPT_CONFIRMATION",
        "command":"relayWrite",
        "data": {
            "pinNumber": 2,
            "pinValue" : true
        }
  }
```

Пример команды получаемой коммутатором для того что бы реле 2 замкнулось. <br>
Топик : (consumer/request/device123)

```json
  {
        "id":2,
        "believerGuarantee":"RECEIPT_CONFIRMATION",
        "command":"relayWrite",
        "currentTimestamp":1772021717684,
        "data": {
            "pinNumber": 2,
            "pinValue" : true
        }
   }
```

## Blink (замыкание и размыкание реле с заданной периодичностью, заданное количество раз)

Пример команды отправляемой на брокер для того что бы реле 4 раза замкнулось и разомкнулось с периодичностью 2 000 миллисекунд. <br>
Топик : producer/request/device123

```json
  {
        "believerGuarantee":"RECEIPT_CONFIRMATION",
        "command":"blink",
        "data": {
            "pinNumber": 2,
            "period" : 2000,
            "count" : 4
        }
  }
```

Пример команды получаемой коммутатором для того что бы реле 4 раза замкнулось и разомкнулось с периодичностью 2 000 миллисекунд. <br> 
Топик : (consumer/request/device123)

```json
  {
        "id":2,
        "believerGuarantee":"RECEIPT_CONFIRMATION",
        "command":"blink",
        "currentTimestamp":1772021717684,
        "data": {
            "pinNumber": 2,
            "period" : 2000,
            "count" : 4
        }
   }
```
