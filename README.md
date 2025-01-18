# Sistem de udare automatizată a plantelor

  Sistem IoT pentru monitorizarea și controlul unui sistem de irigații, utilizând ESP8266, InfluxDB, Grafana și Flask.

# Funcționalități principale

 *  Monitorizarea în timp real a umidității solului și a nivelului apei din rezervor printr-o interfață web.
 * Control manual al pompei de apă.
 * Atât serverul web, cât și dispozitivele IoT comunică folosind protocolul MQTT, utilizând ca broker MQTT Amazon IoT Core.

## Actuatori
  Pompa de apă

## Senzori
 Sunt integrați 2 senzori, care măsoară umiditatea solului și nivelul apei din rezervor 

 # Conectarea la AWS IoT Core
 ## Server web
  Se realizează conexiunea MQTT:
```
mqtt_connection = mqtt_connection_builder.mtls_from_path(
        endpoint=ENDPOINT,
        port=8883,
        cert_filepath=CLIENT_CERT,
        pri_key_filepath=PRIVATE_KEY,
        ca_filepath=CA_CERT,
        client_id=CLIENT_ID,
        clean_session=False,
        keep_alive_secs=60,
        http_proxy_options=proxy_options,
        on_connection_failure=on_connection_failure,
        on_connection_closed=on_connection_closed)
```
## Dispozitivul IoT
```
 // Connect to NTP server to set time
  NTPConnect();

  // Set CA and client certificate for secure communication
  net.setTrustAnchors(&cert);
  net.setClientRSACert(&client_crt, &key);

  // Connect MQTT client to AWS IoT Core
  client.setServer(MQTT_HOST, 8883);
  client.setCallback(messageReceived);

  Serial.println("Connecting to AWS IoT");

  // Attempt to connect to AWS IoT Core
  while (!client.connect(THINGNAME)) {
    Serial.print(".");
    delay(1000);
  }

  // Check if connection is successful
  if (!client.connected()) {
    Serial.println("AWS IoT Timeout!");
    return;
  }
```

# Referințe
1. https://youtu.be/xZoeJ-osS3g?si=Lw8lMLNcilYTrPPt  – How to Connect Esp8266 to aws IoT Core - (2024)
2. https://github.com/aws/aws-iot-device-sdk-python-v2/blob/main/samples/pubsub.py  - AWS SDK Python
3. https://community.grafana.com/t/how-to-change-refresh-rate-from-5s-to-1s/39008 - Rata de refresh de la 5 la 1 secundă 
