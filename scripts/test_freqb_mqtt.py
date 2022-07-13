from email import message
import json
from pydoc import cli
import paho.mqtt.client as mqtt


def connect():
    def on_connect(client, userdata, flags, rc):
        if rc == 0:
            print("Connected to MQTT Broker!")
        else:
            print("Failed to connect, return code %d\n", rc)

    mqttc = mqtt.Client(client_id="test_bed")
    mqttc.connect("203.159.31.209")
    mqttc.on_connect = on_connect
    return mqttc

def subscribe(client: mqtt, on_message):
    client.subscribe('/dashlight/test/freqb')
    client.on_message = on_message


def on_message(client, userdata, msg):
    data = json.loads(msg.payload)
    d = [str(data[k]) for k in data]
    print(f"\t".join(d) + '\r', end="")

client = connect()
subscribe(client, on_message)


client.loop_forever()