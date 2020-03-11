#include <Arduino.h>
#include <FS.h>
#include <ArduinoJson.h>

#include <IotModule.h>
#include <IotMqtt.h>

void messageReceived(String &topic, String &payload) {
    Logger.infof("Recieved [%s]: %s", topic.c_str(), payload.c_str());
}

void lwMQTTErr(lwmqtt_err_t reason)
{
  if (reason == lwmqtt_err_t::LWMQTT_SUCCESS)
    Logger.debug("Success");
  else if (reason == lwmqtt_err_t::LWMQTT_BUFFER_TOO_SHORT)
    Logger.debug("Buffer too short");
  else if (reason == lwmqtt_err_t::LWMQTT_VARNUM_OVERFLOW)
    Logger.debug("Varnum overflow");
  else if (reason == lwmqtt_err_t::LWMQTT_NETWORK_FAILED_CONNECT)
    Logger.debug("Network failed connect");
  else if (reason == lwmqtt_err_t::LWMQTT_NETWORK_TIMEOUT)
    Logger.debug("Network timeout");
  else if (reason == lwmqtt_err_t::LWMQTT_NETWORK_FAILED_READ)
    Logger.debug("Network failed read");
  else if (reason == lwmqtt_err_t::LWMQTT_NETWORK_FAILED_WRITE)
    Logger.debug("Network failed write");
  else if (reason == lwmqtt_err_t::LWMQTT_REMAINING_LENGTH_OVERFLOW)
    Logger.debug("Remaining length overflow");
  else if (reason == lwmqtt_err_t::LWMQTT_REMAINING_LENGTH_MISMATCH)
    Logger.debug("Remaining length mismatch");
  else if (reason == lwmqtt_err_t::LWMQTT_MISSING_OR_WRONG_PACKET)
    Logger.debug("Missing or wrong packet");
  else if (reason == lwmqtt_err_t::LWMQTT_CONNECTION_DENIED)
    Logger.debug("Connection denied");
  else if (reason == lwmqtt_err_t::LWMQTT_FAILED_SUBSCRIPTION)
    Logger.debug("Failed subscription");
  else if (reason == lwmqtt_err_t::LWMQTT_SUBACK_ARRAY_OVERFLOW)
    Logger.debug("Suback array overflow");
  else if (reason == lwmqtt_err_t::LWMQTT_PONG_TIMEOUT)
    Logger.debug("Pong timeout");
}

void lwMQTTErrConnection(lwmqtt_return_code_t reason)
{
  if (reason == lwmqtt_return_code_t::LWMQTT_CONNECTION_ACCEPTED)
    Logger.debug("Connection Accepted");
  else if (reason == lwmqtt_return_code_t::LWMQTT_UNACCEPTABLE_PROTOCOL)
    Logger.debug("Unacceptable Protocol");
  else if (reason == lwmqtt_return_code_t::LWMQTT_IDENTIFIER_REJECTED)
    Logger.debug("Identifier Rejected");
  else if (reason == lwmqtt_return_code_t::LWMQTT_SERVER_UNAVAILABLE)
    Logger.debug("Server Unavailable");
  else if (reason == lwmqtt_return_code_t::LWMQTT_BAD_USERNAME_OR_PASSWORD)
    Logger.debug("Bad UserName/Password");
  else if (reason == lwmqtt_return_code_t::LWMQTT_NOT_AUTHORIZED)
    Logger.debug("Not Authorized");
  else if (reason == lwmqtt_return_code_t::LWMQTT_UNKNOWN_RETURN_CODE)
    Logger.debug("Unknown Return Code");
}

uint8_t IotMqtt::publishMessage(String message) {
    if (!client.connected()) {
        return MQTT_NOT_CONNECTED;
    }

    if (client.publish(pubTopic, message)) {
        return MQTT_SUCCESS;
    }

    return MQTT_FAILURE;
}

void IotMqtt::init() {
    cert.append(cacert);
    client_crt.append(client_cert);
    key.parse(privkey);
    hostname = "a1r32q860r2zlh-ats.iot.us-east-2.amazonaws.com";
    port = 8883;
    clientId = "myIotThing";
    subTopic = "outTopic";
    pubTopic = "inTopic";

    net.setTrustAnchors(&cert);
    net.setClientRSACert(&client_crt, &key);

    client.begin(hostname.c_str(), port, net);
    client.onMessage(messageReceived);
}

void IotMqtt::connect() {
    Logger.debug("MQTT connecting ");
    if (client.connect(clientId.c_str()))
    {
        Logger.debug("connected!");
        if (!client.subscribe(subTopic))
            lwMQTTErr(client.lastError());
    }
    else
    {
        Logger.debugf("SSL Error Code: %i failed, reason ->", net.getLastSSLError());
        lwMQTTErrConnection(client.returnCode());
    }
}

void IotMqtt::handle() {
    if (client.connected()) {
        client.loop();
    } else {
        connect();
    }
}

IotMqtt Mqtt;