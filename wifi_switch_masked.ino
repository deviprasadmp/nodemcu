#include <ESP8266WiFi.h>
#include <ESP8266WebServerSecure.h>

// #define SERIAL_DEBUG

// WiFi
const char* ssid = "<ssid>";
const char* password = "<wep/wpa key>";

// SSL
static const char serverCert[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
<cert data>
-----END CERTIFICATE-----
)EOF";

static const char serverKey[] PROGMEM = R"EOF(
-----BEGIN PRIVATE KEY-----
<cert key>
-----END PRIVATE KEY-----
)EOF";

BearSSL::ESP8266WebServerSecure server(8443);
BearSSL::ServerSessions serverCache(5);

const String API_KEY = "api-key";

void ledOn() {

  digitalWrite(LED_BUILTIN, LOW);
}

void ledOff() {

  digitalWrite(LED_BUILTIN, HIGH);
}

void blink(int times) {

  ledOff();

  for (int i = 0; i < times; i++) {

    ledOn();
    delay(250);
    ledOff();
    delay(250);
  }
}

void relay0On() {

  digitalWrite(1, LOW);
}

void relay0Off() {

  digitalWrite(1, HIGH);
}

void relay1On() {

  digitalWrite(3, LOW);
}

void relay1Off() {

  digitalWrite(3, HIGH);
}

void relay0OnOff(int delayMs) {

  relay0On();
  delay(delayMs);
  relay0Off();
  blink(3);
}

void relay1OnOff(int delayMs) {

  relay1On();
  delay(delayMs);
  relay1Off();
  blink(3);
}

void setup() {

  // GPIO
  pinMode(LED_BUILTIN, OUTPUT);  // Initialize the LED_BUILTIN pin as an output
  pinMode(1, OUTPUT);            // GPIO1
  pinMode(3, OUTPUT);            // GPIO3

  digitalWrite(LED_BUILTIN, HIGH);
  relay0Off();
  relay1Off();

  // WiFi
  /* 
  Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
  would try to act as both a client and an access-point and could cause
  network-issues with your other WiFi-devices on your WiFi-network. 
  */
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {

    blink(1);
  }

  delay(3000);
  blink(5);

  server.getServer().setRSACert(new BearSSL::X509List(serverCert), new BearSSL::PrivateKey(serverKey));
  // Cache SSL sessions to accelerate the TLS handshake.
  server.getServer().setCache(&serverCache);

  server.on("/", handleRequest);
  server.begin();

#ifdef SERIAL_DEBUG
  Serial.begin(9600);
  Serial.println("HTTPS server started ...");
  Serial.println(WiFi.localIP());
#endif
}


/*
Main request handler
*/
void handleRequest() {

  /**
   Read request parameters.
   Ideally, this should have been a POST!
   **/
  const String key = server.arg("key");
  const String relay = server.arg("relay");
  const String delayStr = server.arg("delay");

#ifdef SERIAL_DEBUG
  Serial.printf("key=%s\n", key);
  Serial.printf("relay=%s\n", relay);
  Serial.printf("delayStr=%s\n", delayStr);
#endif

  if (key != API_KEY) {

    return;
  }

  // Brutally disable caching
  server.sendHeader("Cache-Control", "max-age=0, no-cache, must-revalidate, proxy-revalidate, no-store");

  int delayMs = 0;
  if (!delayStr.isEmpty()) {

    delayMs = delayStr.toInt();
  }

  if (delayMs < 1000) {

    delayMs = 1000;
  }

#ifdef SERIAL_DEBUG
  Serial.printf("delayMs=%d\n", delayMs);
#endif


  if (relay == "0") {

#ifdef SERIAL_DEBUG
    Serial.println("Relay 0");
#endif

    server.send(200, "text/plain", "Relay 0\n");
    relay0OnOff(delayMs);
  } else if (relay == "1") {

#ifdef SERIAL_DEBUG
    Serial.println("Relay 1");
#endif

    server.send(200, "text/plain", "Relay 1\n");
    relay1OnOff(delayMs);
  } else {

#ifdef SERIAL_DEBUG
    Serial.println("Unknown relay");
#endif

    server.send(500, "text/plain", "Unknown relay\n");
  }
}

/*
Event loop
*/
void loop() {

  server.handleClient();
}
