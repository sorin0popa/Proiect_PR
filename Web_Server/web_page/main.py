# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0.
# https://github.com/aws/aws-iot-device-sdk-python-v2/blob/main/samples/pubsub.py


from datetime import datetime, timedelta
import pytz
from flask import Flask, request, jsonify, render_template
from flask_sqlalchemy import SQLAlchemy
from sqlalchemy import cast, Numeric, func
from os import environ
from datetime import datetime
# from marshmallow import Schema, fields, ValidationError

from awscrt import mqtt, http
from awsiot import mqtt_connection_builder
from influxdb_client import InfluxDBClient
from influxdb_client.client.write_api import SYNCHRONOUS
import sys
import threading
import time
import json


# Pentru gmail
import smtplib
from email.mime.text import MIMEText
from email.mime.multipart import MIMEMultipart
import os




# This sample uses the Message Broker for AWS IoT to send and receive messages
# through an MQTT connection. On startup, the device connects to the server,
# subscribes to a topic, and begins publishing messages to that topic.
# The device should receive those same messages back from the message broker,
# since it is subscribed to that same topic.

# cmdData is the arguments/input from the command line placed into a single struct for
# use in this sample. This handles all of the command line parsing, validating, etc.
# See the Utils/CommandLineUtils for more information.

ENDPOINT = "a132pc9bxacqas-ats.iot.us-east-1.amazonaws.com"
CLIENT_ID = "iotconsole-6280dc9c-9c8a-444d-a341-0d725b294922"
CA_CERT ="../../AWS_IOT_Certificates/amazon_root_ca1.pem"
CLIENT_CERT = "../../AWS_IOT_Certificates/certificate.pem.crt"
PRIVATE_KEY = "../../AWS_IOT_Certificates/private.pem.key"
SUBSCRIBE_TOPIC_1 = "ESP8266/umiditate_sol"
SUBSCRIBE_TOPIC_2 = "ESP8266/nivel_apa"
SUBSCRIBE_TOPIC_3 = "ESP8266/stare_pompa"
PUBLISH_TOPIC = "web_server/command"
MESSAGE = "Hello, World!"
PORT=8883

# InfluxDB details
BUCKET = "my-bucket"
ORGANIZATION = "my-org"
TOKEN = "token"


app = Flask(__name__)


# Connecting to influxDB
client = InfluxDBClient(url="http://localhost:8086", token=TOKEN, org=ORGANIZATION)

write_api = client.write_api(write_options=SYNCHRONOUS)
query_api = client.query_api()

input_count=10

received_count = 0
received_all_event = threading.Event()

soil_moisture=0
water_level=0
pump_state='off'
last_checked=0
publish_count=0


# for gmail
threshold = 25
time_delay = 60 * 5
water_last_sent_time = 0
soil_last_sent_time = 0

def send_email(SUBJECT=None, BODY=None):
    
    sender_password = "juzo adpw ttqi kmme"

    sender_email = "alexsoryn0@gmail.com"
    recipient_email = "strategikon2001@gmail.com"
    # subject = "Test Email from Python"
    # body = "This is a test email sent using Python and Gmail's SMTP server."
    
    message = MIMEMultipart()
    message["From"] = sender_email
    message["To"] = recipient_email
    message["Subject"] = SUBJECT
    message.attach(MIMEText(BODY, "plain"))
    
    try:
        with smtplib.SMTP("smtp.gmail.com", 587) as server:
            server.starttls()
            server.login(sender_email, sender_password)
            server.sendmail(sender_email, recipient_email, message.as_string())
            print("Email sent successfully!")
    except Exception as e:
        print(f"Failed to send email: {e}")

# Callback when connection is accidentally lost.
def on_connection_interrupted(connection, error, **kwargs):
    pass
    # print("Connection interrupted. error: {}".format(error))


# Callback when an interrupted connection is re-established.
def on_connection_resumed(connection, return_code, session_present, **kwargs):
    # print("Connection resumed. return_code: {} session_present: {}".format(return_code, session_present))

    if return_code == mqtt.ConnectReturnCode.ACCEPTED and not session_present:
        print("Session did not persist. Resubscribing to existing topics...")
        resubscribe_future, _ = connection.resubscribe_existing_topics()

        # Cannot synchronously wait for resubscribe result because we're on the connection's event-loop thread,
        # evaluate result with a callback instead.
        resubscribe_future.add_done_callback(on_resubscribe_complete)


def on_resubscribe_complete(resubscribe_future):
    resubscribe_results = resubscribe_future.result()
    print("Resubscribe results: {}".format(resubscribe_results))

    for topic, qos in resubscribe_results['topics']:
        if qos is None:
            sys.exit("Server rejected resubscribe to topic: {}".format(topic))


# Callback when the subscribed topic receives a message
def on_message_received(topic, payload, dup, qos, retain, **kwargs):
    global soil_moisture
    global water_level
    global last_checked
    global pump_state

    global received_count
    
    global water_last_sent_time
    global soil_last_sent_time
    global threshold
    
    # print("Received message from topic '{}': {}".format(topic, payload))
    
    station = topic.split('/')[0]
    key = topic.split('/')[1]
    value = 0.0
    
    received_count += 1
    
    data = json.loads(payload)
    
    if 'Soil moisture humidity' in data:
        soil_moisture = data['Soil moisture humidity']
        value = soil_moisture
        
        if soil_moisture < threshold and (time.time() - soil_last_sent_time) >= time_delay:
            subj = f"WATER LEVEL for {station} ALERT"
            body = f"Water level is very low!! \n \n Details: \n\n station: {station} \n key: {key} \n value: {float(value)} \n timestamp: {last_checked} \n \n Please take measures. \n\n Thank you."
            
            send_email(SUBJECT=subj, BODY=body)
            soil_last_sent_time = time.time()

    if 'Water level' in data:
        water_level = data['Water level']
        value = water_level
        
        if water_level < threshold and (time.time() - water_last_sent_time) >= time_delay:
            subj = f"WATER LEVEL for {station} ALERT"
            body = f"Water level is very low!! \n \n Details: \n\n station: {station} \n key: {key} \n value: {float(value)} \n timestamp: {last_checked} \n \n Please take measures. \n\n Thank you."
            
            send_email(SUBJECT=subj, BODY=body)
            water_last_sent_time = time.time()
    
    if 'Time' in data:
        last_checked = data['Time']
    
    if 'Pump state' in data:
        if data['Pump state'] == 1:
            pump_state = 'on'
            value = 1
        else:
            pump_state = 'off'
            value = 0
    
    if isinstance(value, float) == True or isinstance(value, int) == True:
        
        db_data = [{
            "measurement": f'{station}.{key}',
            "tags": {
                "station": station,
                "key": key
            },
            "fields": {
                "value": float(value)
            },
            "timestamp": last_checked
        }]
    
        write_api.write(BUCKET, ORGANIZATION, db_data)

    
    # send a notification if is the case
    

# Callback when the connection successfully connects
def on_connection_success(connection, callback_data):
    pass
    # assert isinstance(callback_data, mqtt.OnConnectionSuccessData)
    # print("Connection Successful with return code: {} session present: {}".format(callback_data.return_code, callback_data.session_present))

# Callback when a connection attempt fails
def on_connection_failure(connection, callback_data):
    assert isinstance(callback_data, mqtt.OnConnectionFailureData)
    print("Connection failed with error code: {}".format(callback_data.error))

# Callback when a connection has been disconnected or shutdown successfully
def on_connection_closed(connection, callback_data):
    print("Connection closed")

def subscribe(topic):
    print(f"Subscribing to topic '{topic}'...")
    subscribe_future, packet_id = mqtt_connection.subscribe(
        topic=topic,
        qos=mqtt.QoS.AT_LEAST_ONCE,
        callback=on_message_received)

    subscribe_result = subscribe_future.result()
    print(f"Subscribed with {str(subscribe_result['qos'])}")

def publish(message, topic):
    global publish_count
    # print(f"Publishing message to topic '{topic}': {message}")
    # message_json = json.dumps(message)
    mqtt_connection.publish(
                topic=PUBLISH_TOPIC,
                payload=message,
                qos=mqtt.QoS.AT_LEAST_ONCE)
    time.sleep(1)
    publish_count += 1

# Ruta principala pentru vizualizare date
@app.route("/")
def index():
    
    return render_template("index.html", soil_moisture=soil_moisture, water_level=water_level, pump_state=pump_state,
                            last_checked=last_checked)


# Ruta pentru control manual (ex. pornire pompa)
@app.route("/api/control", methods=["POST"])
def control_pump():
    global pump_state
    
    action = request.json.get("action")
    if action == "start_pump":
        message = "1"
        pump_state = 'on'
        publish(message, PUBLISH_TOPIC)
        return jsonify({"soil_moisture": soil_moisture, "water_level": water_level,
                    "pump_state": pump_state, "last_checked": last_checked,
                    "status": "Pump activated."},
                   ), 200
    elif action == "stop_pump":
        message = "0"
        pump_state = 'off'
        publish(message, PUBLISH_TOPIC)
        return jsonify({"soil_moisture": soil_moisture, "water_level": water_level,
                    "pump_state": pump_state, "last_checked": last_checked,
                    "status": "Pump stopped."},
                    ), 200
    else:
        return jsonify({"status": "Invalid action"}), 400

@app.route("/api/data", methods=["GET"])
def get_current_data():
    global last_checked
   # print(f"PUMP STATE {pump_state}")
    return jsonify({"soil_moisture": soil_moisture, "water_level": water_level,
                    "pump_state": pump_state, "last_checked": last_checked},
                   ), 200

if __name__ == '__main__':
    # Create the proxy options if the data is present in cmdData
    proxy_options = None
    #if cmdData.input_proxy_host is not None and cmdData.input_proxy_port != 0:
    #    proxy_options = http.HttpProxyOptions(
    #        host_name=cmdData.input_proxy_host,
    #        port=cmdData.input_proxy_port)
    
    # Create a MQTT connection from the command line data
    mqtt_connection = mqtt_connection_builder.mtls_from_path(
        endpoint=ENDPOINT,
        port=8883,
        cert_filepath=CLIENT_CERT,
        pri_key_filepath=PRIVATE_KEY,
        ca_filepath=CA_CERT,
        on_connection_interrupted=on_connection_interrupted,
        on_connection_resumed=on_connection_resumed,
        client_id=CLIENT_ID,
        clean_session=False,
        keep_alive_secs=60,
        http_proxy_options=proxy_options,
        on_connection_success=on_connection_success,
        on_connection_failure=on_connection_failure,
        on_connection_closed=on_connection_closed)

    #if not cmdData.input_is_ci:
    #    print(f"Connecting to {cmdData.input_endpoint} with client ID '{cmdData.input_clientId}'...")
    #else:
    print("Connecting to endpoint with client ID")
    connect_future = mqtt_connection.connect()

    # Future.result() waits until a result is available
    connect_future.result()
    print("Connected!")

    message_count = 0

    # Subscribe
    subscribe(SUBSCRIBE_TOPIC_1)
    subscribe(SUBSCRIBE_TOPIC_2)
    subscribe(SUBSCRIBE_TOPIC_3)

    
    app.run(host="0.0.0.0", port=6000, debug=False)
    
    # Wait for all messages to be received.
    # This waits forever if count was set to 0.
    if message_count != 0 and not received_all_event.is_set():
        print("Waiting for all messages to be received...")

    received_all_event.wait()
    print("{} message(s) received.".format(received_count))
    
    # Disconnect
    print("Disconnecting...")
    disconnect_future = mqtt_connection.disconnect()
    disconnect_future.result()
    print("Disconnected!")
    
    