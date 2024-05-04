#include <DHT.h>
#include <LWiFi.h>
#include <NTPClient.h>
#include <PubSubClient.h>
#include <WiFiUdp.h>
#include <Wire.h>

#include "SI114X.h"
#include "U8g2lib.h"

#define DHTPIN A0
#define MPIN A1
#define exButton 2
#define inButton 6
#define waterPump 10
#define DHTTYPE DHT11

const char* irrigateCause[] = {"Moisture", "DHT11", "SI114X"};
int irrigateCauseIndex = 0;
#define TEMPERATURE_THRESHOLD_HIGH 30.0
#define TEMPERATURE_THRESHOLD_LOW 15.0
#define HUMIDITY_THRESHOLD 50
#define MOISTURE_THRESHOLD 50
#define LIGHT_THRESHOLD 300
#define IR_THRESHOLD 600
#define UV_THRESHOLD 300
const int irrigationTime = 3;
int irrigationWork = 0;
bool irrigateStart = false;

DHT dht(DHTPIN, DHTTYPE);
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
SI114X SI = SI114X();

int exButtonState = 0;
int inButtonState = 0;
char ssid[] = "ssid";
char pass[] = "passwd";
String lineToken = "";
const int lineDelay = 100;
int lineWait = 99;
int status = WL_IDLE_STATUS;
const char* mqttServer = "LinkToMQTTServer";
const char* clientID = "ID";
const char* mqttUserName = "";
const char* mqttPwd = "";
const char* topic = "TopicForPosting";
const char* calltopic = "TopicForReceiving";
const char* SI_topic[] = {"visible", "ir", "uv"};
const char* DHT_topic[] = {"humidity", "celsius", "fahrenheit"};
const int HISTORY_SIZE = 5;
const char* DHT_symbol[] = {"%", "°C", "°F"};
const char* M_topic = "moisture";
unsigned long prevMillis = 0;
const long interval = 1;
String msgStr = "";

WiFiClient LinkitClient;
PubSubClient client(LinkitClient);
WiFiUDP ntpUDP;
IPAddress ip;
WiFiServer server(80);

bool ledState = false;
bool displayDHT = true;

bool freezeState = false;

int history[HISTORY_SIZE][3];
int historyIndex = 0;

NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 60 * 60 * 8, 60000);

int monthLengths[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
// Converts epoch time to formatted string representation (e.g., "Current time : 2022/4/2")
String convertEpochTime(long epoch) {
    String temp = "";
    long years, year, months = 1, days;
    days = epoch / (24 * 60 * 60);
    years = days / 365 + 1970;
    year = 1970;
    days %= 365;

    while (year <= years) {
        if (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0))
            days--;
        year++;
    }

    while (monthLengths[months - 1] < days) {
        days -= monthLengths[months - 1];
        months++;
    }

    temp = "Current time : " + String(years) + "/" + String(months) + "/" + String(days + 1);

    return temp;
}

// Connects to the Wi-Fi network with the given SSID and password
void connectWiFi() {
    while (status != WL_CONNECTED) {
        Serial.print("Attempting to connect to WPA SSID: ");
        Serial.println(ssid);

        status = WiFi.begin(ssid, pass);
        delay(1000);
    }

    ip = WiFi.localIP();
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(ip);
}

// Attempts to reconnect to MQTT if the connection has been lost
void reconnectWiFi() {
    Serial.println("MQTT not connected");
    while (!client.connected()) {
        if (client.connect(clientID, mqttUserName, mqttPwd)) {
            Serial.println("MQTT connected");
        } else {
            Serial.print("failed, rc = ");
            Serial.print(client.state());
            Serial.println(" try again in 1 seconds");
            delay(1000);
        }
    }
}

// Callback function to handle incoming MQTT messages
void callback(char* calltopic, byte* payload, unsigned int length) {
    if ((char)payload[0] != 'o' && (char)payload[0] != 'O')
        return;

    String message;
    for (int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    Serial.print("Message arrived [");
    Serial.print(calltopic);
    Serial.print("]");

    if (message == "ON" || message == "on") {
        digitalWrite(LED_BUILTIN, HIGH);
    } else if (message == "OFF" || message == "off") {
        digitalWrite(LED_BUILTIN, LOW);
    }

    Serial.println(message);
}

// Publishes light sensor (SI114X) readings to MQTT topics
void mqttSI114X(int SI_value[]) {
    int i = 0;
    if (millis() - prevMillis > interval) {
        prevMillis = millis();
        for (; i < 3; i++) {
            Serial.print(String(SI_topic[i]) + ": " + SI_value[i] + " ");
            client.publish((String(topic) + SI_topic[i]).c_str(), String(SI_value[i]).c_str());
        }
    }
}

// Publishes DHT sensor readings to MQTT topics
void mqttDHT(int DHT_value[]) {
    int i = 0;
    if (millis() - prevMillis > interval) {
        prevMillis = millis();
        for (; i < 3; i++) {
            Serial.print(String(DHT_topic[i]) + ": " + DHT_value[i] + " ");
            client.publish((String(topic) + DHT_topic[i]).c_str(), String(DHT_value[i]).c_str());
        }
    }
}

// Publishes moisture readings to MQTT
void mqttM(int M_value) {
    if (millis() - prevMillis > interval) {
        prevMillis = millis();
        Serial.print(String(M_topic) + ": " + M_value + " ");
        client.publish((String(topic) + M_topic).c_str(), String(M_value).c_str());
    }
}

// Sends a custom string message over MQTT
void mqttStr(String msgStr) {
    if (millis() - prevMillis > interval) {
        prevMillis = millis();

        byte arrSize = msgStr.length() + 1;
        char msg[arrSize];

        msgStr.toCharArray(msg, arrSize);
        client.publish(topic, msg);
        msgStr = "";
    }
}

// Starts the display (u8g2 library) and initializes the settings
void startu8g2() {
    u8g2.begin();
    u8g2.setFont(u8g2_font_ncenB10_tr);
    u8g2.setDrawColor(1);
    u8g2.setFont(u8g2_font_ncenB14_tr);
    u8g2.enableUTF8Print();
    u8g2.setFont(u8g2_font_unifont_t_chinese2);
    u8g2.clear();
}

// Initiates and checks for SI1145 UV sensor connections
void startSI1145() {
    while (!SI.Begin()) {
        Serial.println("Couldn't find SI1145 sensor!");
        u8g2.clear();
        u8g2.setCursor(0, 15);
        u8g2.print("Couldn't find SI1145 sensor!");
        u8g2.sendBuffer();
        delay(1000);
    }
    Serial.println("SI1145 found!");
}

// Updates the display with the DHT sensor readings
void u8g2DHT(int DHT_value[]) {
    int i = 0;
    u8g2.clear();
    u8g2.setCursor(0, 15);
    u8g2.print("DHT11: ");
    for (; i < 3; i++) {
        u8g2.setCursor(0, 31 + i * 16);
        u8g2.print(String(DHT_topic[i]) + ": " + String(DHT_value[i]) + DHT_symbol[i]);
    }
    u8g2.sendBuffer();
}

// Updates the display with the SI114X sensor readings
void u8g2SI114X(int SI_value[]) {
    int i = 0;
    u8g2.clear();
    u8g2.setCursor(0, 15);
    u8g2.print("SI1145: ");
    for (; i < 3; i++) {
        u8g2.setCursor(0, 31 + i * 16);
        u8g2.print(String(SI_topic[i]) + ": " + String(SI_value[i]));
    }
    u8g2.sendBuffer();
}

// Updates the display with soil moisture readings
void u8g2M(int M_value) {
    u8g2.clear();
    u8g2.setCursor(0, 15);
    u8g2.print("Moisture value: ");
    u8g2.setCursor(0, 31);
    u8g2.print(M_value);
    u8g2.sendBuffer();
}

// Function to send a message via LINE Notify
void sendLineMsg(String myMsg) {
    static TLSClient line_client;
    myMsg = "message=" + myMsg;
    myMsg.replace("\n", "%0A");
    Serial.println("Connecting to LINE Notify...");
    if (line_client.connect("notify-api.line.me", 443)) {
        Serial.println("Connected to LINE Notify");
        line_client.println("POST /api/notify HTTP/1.1");
        line_client.println("Connection: close");
        line_client.println("Host: notify-api.line.me");
        line_client.println("Authorization: Bearer " + lineToken);
        line_client.println("Content-Type: application/x-www-form-urlencoded");
        line_client.println("Content-Length: " + String(myMsg.length()));
        line_client.println();
        line_client.println(myMsg);
        line_client.println();
        line_client.stop();
    } else {
        Serial.println("Connection failed");
    }
}

// Stores the DHT readings in an array to keep a history
void storeDHTReadings(int DHT_value[]) {
    int i = 0;
    for (; i < 3; i++)
        history[historyIndex][i] = DHT_value[i];

    historyIndex = (historyIndex + 1) % HISTORY_SIZE;
}

// Sends an HTML constructed table of DHT history readings to a connected client
void displayHistory(WiFiClient& client) {
    client.println("<table>");
    client.println("<tr><th>Humidity (%)</th><th>Temperature (°C)</th><th>Temperature (°F)</th></tr>");
    for (int i = 0; i < HISTORY_SIZE; i++) {
        int index = (historyIndex + i) % HISTORY_SIZE;
        client.print("<tr><td>");
        client.print(history[index][0]);
        client.print("</td><td>");
        client.print(history[index][1]);
        client.print("</td><td>");
        client.print(history[index][2]);
        client.println("</td></tr>");
    }
    client.println("</table>");
}

// Handles incoming client requests via the HTTP server and responds with the status
void handleHttpServer(int DHT_value[], int SI_value[], int M_value) {
    WiFiClient client = server.available();
    if (client) {
        Serial.println("New client connected");
        String currentLine = "";
        while (client.connected()) {
            if (client.available()) {
                char c = client.read();
                Serial.write(c);
                if (c == '\n') {
                    if (currentLine.length() == 0) {
                        client.println("HTTP/1.1 200 OK");
                        client.println("Content-type:text/html");
                        client.println();

                        client.println("<h1>Welcome to Simple Linkit 7697 control panel</h1>");

                        client.println("<form action=\"/\" method=\"get\">");
                        client.println("LED State: <select name=\"led_state\" onchange=\"this.form.submit();\">");
                        client.println("<option value=\"on\">On</option>");
                        client.println("<option value=\"off\"" + String(!ledState ? " selected" : "") + ">Off</option>");
                        client.println("</select>");

                        client.println("DHT Display: <select name=\"dht_display\" onchange=\"this.form.submit();\">");
                        client.println("<option value=\"on\">On</option>");
                        client.println("<option value=\"off\"" + String(!displayDHT ? " selected" : "") + ">Off</option>");
                        client.println("</select>");

                        client.println("Freeze: <select name=\"freeze_state\" onchange=\"this.form.submit();\">");
                        client.println("<option value=\"off\">Off</option>");
                        client.println("<option value=\"on\"" + String(freezeState ? " selected" : "") + ">On</option>");
                        client.println("</select>");

                        client.println("<input type=\"submit\" value=\"Submit\">");
                        client.println("</form>");

                        if (displayDHT) {
                            storeDHTReadings(DHT_value);
                            client.print("<p>Humidity: ");
                            client.print(DHT_value[0]);
                            client.print("%, Temperature: ");
                            client.print(DHT_value[1]);
                            client.print("°C / ");
                            client.print(DHT_value[2]);
                            client.println("°F</p>");

                            displayHistory(client);
                        }

                        client.println();
                        break;
                    } else {
                        currentLine = "";
                    }
                } else if (c != '\r') {
                    currentLine += c;
                }

                int paramStart = currentLine.indexOf('?');
                if (paramStart != -1) {
                    String params = currentLine.substring(paramStart + 1);
                    int ampersandIndex;
                    while ((ampersandIndex = params.indexOf('&')) != -1) {
                        String paramPair = params.substring(0, ampersandIndex);
                        int equalIndex = paramPair.indexOf('=');
                        String paramName = paramPair.substring(0, equalIndex);
                        String paramValue = paramPair.substring(equalIndex + 1);

                        if (paramName == "freeze_state") {
                            freezeState = (paramValue == "on");
                        } else if (paramName == "led_state") {
                            ledState = (paramValue != "off");
                            digitalWrite(LED_BUILTIN, ledState);
                        } else if (paramName == "dht_display") {
                            displayDHT = (paramValue != "off");
                        }

                        params = params.substring(ampersandIndex + 1);
                    }

                    int equalIndex = params.indexOf('=');
                    if (equalIndex != -1) {
                        String paramName = params.substring(0, equalIndex);
                        String paramValue = params.substring(equalIndex + 1);

                        if (paramName == "freeze_state") {
                            freezeState = (paramValue == "on");
                        } else if (paramName == "led_state") {
                            ledState = (paramValue != "off");
                            digitalWrite(LED_BUILTIN, ledState);
                        } else if (paramName == "dht_display") {
                            displayDHT = (paramValue != "off");
                        }
                    }
                }
            }
        }
        client.stop();
        Serial.println("Client disconnected");
    }
}

// Handles the freeze mode HTTP server where irrigation and sensor readings are halted
void handleFreezeHttpServer() {
    WiFiClient client = server.available();
    if (client) {
        Serial.println("New client connected");
        String currentLine = "";
        while (client.connected()) {
            if (client.available()) {
                char c = client.read();
                Serial.write(c);
                if (c == '\n') {
                    if (currentLine.length() == 0) {
                        client.println("HTTP/1.1 200 OK");
                        client.println("Content-type:text/html");
                        client.println();

                        client.println("<h1>The Linkit 7697 board is currently freeze</h1>");

                        client.println("<form action=\"/\" method=\"get\">");
                        client.println("Freeze: <select name=\"freeze_state\" onchange=\"this.form.submit();\">");
                        client.println("<option value=\"off\">Off</option>");
                        client.println("<option value=\"on\"" + String(freezeState ? " selected" : "") + ">On</option>");
                        client.println("</select>");

                        client.println("<input type=\"submit\" value=\"Submit\">");
                        client.println("</form>");

                        client.println();
                        break;
                    } else {
                        currentLine = "";
                    }
                } else if (c != '\r') {
                    currentLine += c;
                }

                if (currentLine.startsWith("GET /?freeze_state=on")) {
                    freezeState = true;
                } else if (currentLine.startsWith("GET /?freeze_state=")) {
                    freezeState = false;
                }
            }
        }
        client.stop();
        Serial.println("Client disconnected");
    }
}

// Main irrigation logic that decides when to activate the water pump
void irrigation(int DHT_value[], int SI_value[], int M_value) {
    if (irrigateStart) {
        if (!irrigationWork) {
            digitalWrite(10, LOW);
            sendLineMsg("Irrigation started " + String(irrigateCause[irrigateCauseIndex]));
        }
        irrigationWork++;
        if (irrigationWork == irrigationTime) {
            sendLineMsg("Irrigation ended");
            irrigateStart = false;
            irrigationWork = 0;
        }
        return;
    }
    Serial.println("All value: " + String(SI_value[0]) + " " + String(SI_value[1]) + " " + String(SI_value[2]) + " " + String(DHT_value[0]) + " " + String(DHT_value[1]) + " " + String(DHT_value[2]) + " " + String(M_value));

    if (M_value < MOISTURE_THRESHOLD) {
        irrigateCauseIndex = 0;
        digitalWrite(10, LOW);
        irrigateStart = true;
        return;
    }

    if (DHT_value[0] < HUMIDITY_THRESHOLD) {
        if (DHT_value[1] > TEMPERATURE_THRESHOLD_LOW && DHT_value[1] < TEMPERATURE_THRESHOLD_HIGH) {
            irrigateCauseIndex = 1;
            digitalWrite(10, LOW);
            irrigateStart = true;
            return;
        }
    }

    if (SI_value[1] > IR_THRESHOLD) {
        irrigateCauseIndex = 2;
        digitalWrite(10, LOW);
        irrigateStart = true;
        return;
    }

    digitalWrite(10, HIGH);
}

// Arduino setup function that initializes hardware and network connections
void setup() {
    Serial.begin(9600);
    Wire.begin();
    while (!Serial) {
        ;
    }
    pinMode(exButton, INPUT);
    pinMode(inButton, INPUT);
    pinMode(MPIN, INPUT);
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(waterPump, OUTPUT);
    digitalWrite(waterPump, HIGH);
    dht.begin();
    startSI1145();
    startu8g2();
    Serial.println("Starting...");

    connectWiFi();
    Serial.println("You're connected to the network");

    randomSeed(millis());
    clientID = (clientID + String(random(0xffff), HEX)).c_str();
    client.setServer(mqttServer, 1883);
    client.setCallback(callback);

    reconnectWiFi();

    client.subscribe(calltopic);
    server.begin();
}

// Main loop function where the logic of sensor reading, MQTT communication, and display updating happens
void loop() {
    delay(3000);
    if (!client.connected()) {
        reconnectWiFi();
    }
    client.loop();
    while (!SI.Begin()) {
        Serial.println("Couldn't find SI1145 sensor!");
        delay(500);
    }

    while (freezeState) {
        Serial.println("Web Freeze mode active");
        handleFreezeHttpServer();
        delay(2000);
    }

    if (digitalRead(inButton)) {
        inButtonState += digitalRead(inButton);
        inButtonState %= 2;
        while (inButtonState) {
            Serial.print("Button Freeze!");
            digitalWrite(LED_BUILTIN, HIGH);
            delay(1000);
            inButtonState += digitalRead(inButton);
            inButtonState %= 2;
        }
        digitalWrite(LED_BUILTIN, LOW);
    }

    if (digitalRead(exButton)) {
        exButtonState += digitalRead(exButton);
        Serial.print("Pressed");
    }
    exButtonState %= 3;

    delay(5);
    int DHT_value[] = {dht.readHumidity(), dht.readTemperature(), dht.readTemperature(true)};
    if (isnan(DHT_value[0]) || isnan(DHT_value[1]) || isnan(DHT_value[2])) {
        Serial.println("Failed to read from DHT11 sensor!");
        return;
    }

    int M_value = analogRead(MPIN) / 4;

    int SI_value[] = {SI.ReadVisible(), SI.ReadIR(), SI.ReadUV()};

    if (!exButtonState) {
        u8g2SI114X(SI_value);
        mqttSI114X(SI_value);
    } else if (exButtonState == 1) {
        u8g2DHT(DHT_value);
        mqttDHT(DHT_value);
    } else {
        u8g2M(M_value);
        mqttM(M_value);
    }

    Serial.print(exButtonState);
    Serial.print(" ");
    Serial.print(inButtonState);

    timeClient.update();
    msgStr = convertEpochTime(timeClient.getEpochTime()) + " " + timeClient.getFormattedTime() + " Linkit 7697 is currently working";
    if (lineWait == lineDelay) {
        sendLineMsg(msgStr);
        lineWait = 0;
    } else
        lineWait++;

    irrigation(DHT_value, SI_value, M_value);

    handleHttpServer(DHT_value, SI_value, M_value);

    mqttStr(msgStr);
    Serial.println();
}
