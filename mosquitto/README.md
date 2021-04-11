# mosquitto

MQTT client code using the mosquitto library (libmosquitto):
* `client_sub`: subscribe to topic
* `client_pub`: publish to topic

Building:

```console
$ gcc client_sub.c -o client_sub -lmosquitto
$ gcc client_pub.c -o client_pub -lmosquitto
```
