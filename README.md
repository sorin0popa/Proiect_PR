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
      


# Referinte
1)  https://youtu.be/xZoeJ-osS3g?si=Lw8lMLNcilYTrPPt  – How to Connect Esp8266 to aws IoT Core - (2024) 
(2) https://github.com/aws/aws-iot-device-sdk-python-v2/blob/main/samples/pubsub.py  - AWS SDK Python 
(3) https://community.grafana.com/t/how-to-change-refresh-rate-from-5s-to-1s/39008 - Rata de refresh de la 5 la 1 secundă 
