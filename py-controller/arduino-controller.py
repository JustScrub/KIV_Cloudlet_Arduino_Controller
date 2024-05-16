#!/home/admin/python/bin/python3

from keyhole import Keyhole
from flask import Flask
import requests as req

APP_CONTEXT = '/api'

app = Flask(__name__)
arduino = None

def initArduino(serial_port):
    k = Keyhole( serial_port )
    return k

@app.route(f'{APP_CONTEXT}/ping')
def ping():
    arduino << 'ping\n'
    return arduino.read()

@app.route(f'{APP_CONTEXT}/fan1/<val>')
def setFan1(val):
    arduino << f'fan1={val}'
    return 'ok'

@app.route(f'{APP_CONTEXT}/fan2/<val>')
def setFan2(val):
    arduino << f'fan2={val}'
    return 'ok'

@app.route(f'{APP_CONTEXT}/fan3/<val>')
def setFan3(val):
    arduino << f'fan3={val}'
    return 'ok'

@app.route(f'{APP_CONTEXT}/led/<val>')
def setLed(val):
    arduino << f'led={val}'
    return 'ok'

@app.route(f'{APP_CONTEXT}/reveal_node/<id>')
def revealNode(id):
    if id < 1 or id > 5:
        return 'error'
    node_url = f'http://10.88.99.{id}:8088/api/identify'
    response = req.get(node_url)
    return 'ok'

if __name__ == '__main__':
    arduino = initArduino('/dev/ttyACM0')
    app.run(host="0.0.0.0", port=8080)
