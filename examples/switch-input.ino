#include "config.h"

#if CLIENT_SECURE == 1
#include "secure-credentials.h"
#endif
// const char* sensors[][60] = {
//   { "3306", "1", "5850", "digital_input"},
//   { "3342", "2", "5850", "digital_output"},
// };

#include <AloesDevice.h>

#include <Bounce2.h>

#define BUTTON_PIN_1 D5

unsigned long releaseInterval = 1000;
int buttonState1;
unsigned long buttonTimeStamp1;

Bounce debouncer1 = Bounce();

void setupPins(Bounce *instance, int pin);
void readButton(Bounce *instance, int btnState, unsigned long timestamp);

void setupPins(Bounce *instance, int pin) {
  instance->attach(pin, INPUT_PULLUP);
  instance->interval(releaseInterval);
  //  debouncer1.attach(BUTTON_PIN_1, INPUT_PULLUP);
  //  debouncer1.interval(releaseInterval);
}

void readButton(Bounce *instance, int btnState, unsigned long timestamp) {
  boolean changed = instance->update();
  // adapt sensor index to your own sensors.json
  int objectId = aloes.device->sensors->get(0, S_OBJECT_ID);
  int nodeId = aloes.device->sensors->get(0, S_NODE_ID);
  int sensorId = aloes.device->sensors->get(0, S_SENSOR_ID);
  int resourceId = aloes.device->sensors->get(0, S_RESOURCE_ID);

  // if ( changed ) {
  int value = instance->read();
  if (value == HIGH && buttonState1 == 0) {
    buttonState1 = 0;
    if (millis() - buttonTimeStamp1 >= releaseInterval) {
      buttonTimeStamp1 = millis();
      char newPayload[5];
      itoa(buttonState1, newPayload, 10);
      aloes.setMsg(METHOD, "1")
          .setMsg(OBJECT_ID, objectId)
          .setMsg(NODE_ID, nodeId)
          .setMsg(SENSOR_ID, sensorId)
          .setMsg(RESOURCE_ID, resourceId)
          .setMsg(PAYLOAD, newPayload);
      aloes.sendMessage(MQTT);
      return;
    }
  } else if (value == HIGH && buttonState1 == 1) {
    buttonState1 = 0;
    aSerial.vvv().pln(F("Button released"));
    char newPayload[5] = "0";
    itoa(buttonState1, newPayload, 10);
    aloes.setMsg(METHOD, "1")
        .setMsg(OBJECT_ID, objectId)
        .setMsg(NODE_ID, nodeId)
        .setMsg(SENSOR_ID, sensorId)
        .setMsg(RESOURCE_ID, resourceId)
        .setMsg(PAYLOAD, newPayload);
    aloes.sendMessage(MQTT);
    return;
  } else if (value == LOW && buttonState1 == 0) {
    buttonState1 = 1;
    aSerial.vvv().p(F("Button pushed : ")).pln(buttonState1);
    buttonTimeStamp1 = millis();
  } else if (value == LOW && buttonState1 == 1) {
    buttonState1 = 1;
    if (millis() - buttonTimeStamp1 >= releaseInterval) {
      buttonTimeStamp1 = millis();
      char newPayload[5];
      itoa(buttonState1, newPayload, 10);
      aSerial.vvv()
          .p(F("Retriggering button : "))
          .p(newPayload)
          .p("  ")
          .pln(sensorId);
      aloes.setMsg(METHOD, "1")
          .setMsg(OBJECT_ID, objectId)
          .setMsg(NODE_ID, nodeId)
          .setMsg(SENSOR_ID, sensorId)
          .setMsg(RESOURCE_ID, resourceId)
          .setMsg(PAYLOAD, newPayload);
      aloes.sendMessage(MQTT);
      return;
    }
  }
  // }
}

//  CALLED on errors
void onError(modules moduleName, char *error) {
  //  if (moduleName == EXEC) {
  //    aSerial.vv().p(F("[EXEC] error : ")).pln(error);
  //  } else if (moduleName == TRANSPORT) {
  //    aSerial.vv().p(F("[TRANSPORT] error : ")).pln(error);
  //  }
}

// CALLED on incoming http/mqtt/serial message
// MQTT pattern = "prefixedDevEui/+method/+objectId/+sensorId/+resourceId",
// HTTP pattern = "apiRoot/+collection/+path/#param"
void onMessage(transportLayer type, Message *message) {
  if (type == MQTT) {
    char *payload = message->get(PAYLOAD);
    const char *method = message->get(METHOD);
    const char *objectId = message->get(OBJECT_ID);
    const char *resourceId = message->get(RESOURCE_ID);
    const char *nodeId = message->get(NODE_ID);
    const char *sensorId = message->get(SENSOR_ID);
    if (strcmp(objectId, "3306") == 0 && strcmp(resourceId, "5850") == 0) {
      if (strcmp(method, "1") == 0) {
        if ((strcmp(payload, "true") == 0 || strcmp(payload, "1") == 0)) {
          digitalWrite(RELAY_SWITCH, HIGH);
          digitalWrite(STATE_LED, LOW);
        } else if ((strcmp(payload, "false") == 0 ||
                    strcmp(payload, "0") == 0)) {
          digitalWrite(RELAY_SWITCH, LOW);
          digitalWrite(STATE_LED, HIGH);
        }
      } else if (strcmp(method, "2") == 0) {
        int val = digitalRead(RELAY_SWITCH);
        char newPayload[10];
        itoa(val, newPayload, 10);
        message->set(METHOD, "1").set(PAYLOAD, newPayload);
        aloes.sendMessage(MQTT);
      }
    }
    return;
  } else if (type == HTTP) {
    const char *method = message->get(METHOD);
    const char *collection = message->get(COLLECTION);
    const char *path = message->get(PATH);
    //  const char* body = message->get(PAYLOAD);
    //  aSerial.vvv().p(F("onMessage : HTTP : ")).p(collection).p("
    //  ").pln(path);
    return;
  }
}

void setup() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(DEFAULT_WIFI_SSID, DEFAULT_WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  if (!aloesSetup()) {
    return;
  }
  setupPins(debouncer1, BUTTON_PIN_1);
}

void loop() {
  if (!aloesLoop()) {
    return;
  }
  readButton(debouncer1, buttonState1, buttonTimeStamp1);
}