import paho.mqtt.client as mqtt
import telepot

TELEGRAM_TOKEN = '6489709888:AAHP2qccH9d-E9THTgILNumEDpFmj64K_mw'
TELEGRAM_CHAT_ID = 444797793  

bot = telepot.Bot(TELEGRAM_TOKEN)

def on_connect(client, userdata, flags, rc):
    # print("Connected with result code " + str(rc))
    client.subscribe("GABELLA")

def on_message(client, userdata, msg):
    # print("GABELLA", msg.payload)
    bot.sendMessage(TELEGRAM_CHAT_ID, text=msg.payload.decode('utf-8'))

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

client.connect("broker.hivemq.com", 1883, 60)
client.loop_forever()
