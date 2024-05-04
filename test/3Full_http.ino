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
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);  // initialization for the used OLED display
SI114X SI = SI114X();

int exButtonState = 0;
int inButtonState = 0;
char ssid[] = "ssid";    // your network SSID (name)
char pass[] = "passwd";  // your network password
String lineToken = "";
const int linedelay = 100;
int linewait = 99;
int status = WL_IDLE_STATUS;    // the Wifi radio's status
const char* mqttServer = "";    // MQTT server location
const char* clientID = "";      // sid
const char* mqttUserName = "";  // mqtt username
const char* mqttPwd = "";       // MQTT password
const char* topic = "";         // topic
const char* SI_topic[] = {"visible", "ir", "uv"};
const char* DHT_topic[] = {"humidity", "celsius", "fahrenheit"};
const int HISTORY_SIZE = 5;
const char* DHT_symbol[] = {"%", "°C", "°F"};
const char* M_topic = "moisture";
const char* calltopic = "";    // call topic
unsigned long prevMillis = 0;  //
const long interval = 1;       // Time interval
String msgStr = "";            // message

WiFiClient LinkitClient;
PubSubClient client(LinkitClient);
WiFiUDP ntpUDP;
IPAddress ip;
WiFiServer server(80);

// Variables to store checkbox states
bool ledState = false;
bool displayDHT = true;  // Initially display DHT data
// Variable to control freeze state
bool freezeState = false;

// Function to store DHT readings in a circular buffer (history)
int history[HISTORY_SIZE][3];  // 2D array to store h, t, f for each entry
int historyIndex = 0;

// Taipei UTC +8
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 60 * 60 * 8, 60000);

int monthLengths[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
String convertEpochTime(long epoch) {
    String temp = "";
    long years, year, months = 1, days;
    days = epoch / (24 * 60 * 60);
    years = days / 365 + 1970;
    year = 1970;
    days %= 365;

    // Adjust for leap years
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

void connectWiFi() {
    // attempt to connect to Wifi network:
    while (status != WL_CONNECTED) {
        Serial.print("Attempting to connect to WPA SSID: ");
        Serial.println(ssid);
        // Connect to WPA/WPA2 network:
        status = WiFi.begin(ssid, pass);
        delay(1000);
    }

    ip = WiFi.localIP();
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(ip);
}

void reconnectWiFi() {
    Serial.println("MQTT not connected");
    while (!client.connected()) {
        if (client.connect(clientID, mqttUserName, mqttPwd)) {
            Serial.println("MQTT connected");
        } else {
            Serial.print("failed, rc = ");
            Serial.print(client.state());
            Serial.println(" try again in 1 seconds");
            delay(1000);  // wait 1 second
        }
    }
}

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

void mqttM(int M_value) {
    if (millis() - prevMillis > interval) {
        prevMillis = millis();
        Serial.print(String(M_topic) + ": " + M_value + " ");
        client.publish((String(topic) + M_topic).c_str(), String(M_value).c_str());
    }
}

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

void startu8g2() {
    u8g2.begin();
    u8g2.setFont(u8g2_font_ncenB10_tr);
    u8g2.setDrawColor(1);
    u8g2.setFont(u8g2_font_ncenB14_tr);
    u8g2.enableUTF8Print();
    u8g2.setFont(u8g2_font_unifont_t_chinese2);
    u8g2.clear();
}

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

void u8g2M(int M_value) {
    u8g2.clear();
    u8g2.setCursor(0, 15);
    u8g2.print("Moisture value: ");
    u8g2.setCursor(0, 31);
    u8g2.print(M_value);
    u8g2.sendBuffer();
}

void sendLineMsg(String myMsg) {
    static TLSClient line_client;
    myMsg = "message=" + myMsg;
    myMsg.replace("\n", "%0A");  // Encode newline characters
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

void storeDHTReadings(int DHT_value[]) {
    int i = 0;
    for (; i < 3; i++)
        history[historyIndex][i] = DHT_value[i];

    historyIndex = (historyIndex + 1) % HISTORY_SIZE;  // Wrap around
}

// Function to display DHT history as HTML table
void displayHistory(WiFiClient& client) {
    client.println("<table>");
    client.println("<tr><th>Humidity (%)</th><th>Temperature (°C)</th><th>Temperature (°F)</th></tr>");
    for (int i = 0; i < HISTORY_SIZE; i++) {
        int index = (historyIndex + i) % HISTORY_SIZE;  // Get index considering wrap-around
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

// Function to handle HTTP server requests
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
                        // HTML form with dropdown menus
                        client.println("<form action=\"/\" method=\"get\">");
                        client.println("LED State: <select name=\"led_state\" onchange=\"this.form.submit();\">");
                        client.println("<option value=\"on\">On</option>");
                        client.println("<option value=\"off\"" + String(!ledState ? " selected" : "") + ">Off</option>");  // Selected if LED is off
                        client.println("</select>");

                        client.println("DHT Display: <select name=\"dht_display\" onchange=\"this.form.submit();\">");
                        client.println("<option value=\"on\">On</option>");
                        client.println("<option value=\"off\"" + String(!displayDHT ? " selected" : "") + ">Off</option>");  // Selected if DHT is off
                        client.println("</select>");

                        // HTML form with Freeze dropdown
                        client.println("Freeze: <select name=\"freeze_state\" onchange=\"this.form.submit();\">");
                        client.println("<option value=\"off\">Off</option>");
                        client.println("<option value=\"on\"" + String(freezeState ? " selected" : "") + ">On</option>");
                        client.println("</select>");

                        client.println("<input type=\"submit\" value=\"Submit\">");  // Submit button (optional, as onchange submits)
                        client.println("</form>");

                        // Display DHT readings and history only if displayDHT is true
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

                // Check if dropdown options were submitted
                // Parse GET parameters using indexOf and substring
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
                            ledState = (paramValue != "off");  // Assuming "on" or anything else turns it on
                            digitalWrite(LED_BUILTIN, ledState);
                        } else if (paramName == "dht_display") {
                            displayDHT = (paramValue != "off");
                        }

                        params = params.substring(ampersandIndex + 1);
                    }

                    // Check the last parameter (after the last '&')
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
                        // HTML form with dropdown menus
                        // HTML form with Freeze dropdown
                        client.println("<form action=\"/\" method=\"get\">");
                        client.println("Freeze: <select name=\"freeze_state\" onchange=\"this.form.submit();\">");
                        client.println("<option value=\"off\">Off</option>");
                        client.println("<option value=\"on\"" + String(freezeState ? " selected" : "") + ">On</option>");
                        client.println("</select>");

                        client.println("<input type=\"submit\" value=\"Submit\">");  // Submit button (optional, as onchange submits)
                        client.println("</form>");

                        client.println();
                        break;
                    } else {
                        currentLine = "";
                    }
                } else if (c != '\r') {
                    currentLine += c;
                }

                // Check if dropdown options were submitted
                // Check if freeze option was submitted
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

void setup() {
    // Initialize serial and wait for port to open:
    Serial.begin(9600);
    Wire.begin();
    while (!Serial) {
        ;  // wait for serial port to connect. Needed for native USB port only
    }
    pinMode(exButton, INPUT);
    pinMode(inButton, INPUT);
    pinMode(MPIN, INPUT);
    pinMode(LED_BUILTIN, OUTPUT);
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
    msgStr = convertEpochTime(timeClient.getEpochTime()) + " " + timeClient.getFormattedTime();
    if (linewait == linedelay) {
        sendLineMsg(msgStr);
        linewait = 0;
    } else
        linewait++;

    handleHttpServer(DHT_value, SI_value, M_value);

    mqttStr(msgStr);
    Serial.println();
}