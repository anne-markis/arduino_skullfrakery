#include <ESP8266WiFi.h>

#define MAX_PWM 1023 // TODO redundant with PWMRANGE
#define MIN_PWM 0

#define PIN4 D2 // Kind of unnecessary but cool: https://github.com/esp8266/Arduino/blob/master/variants/nodemcu/pins_arduino.h
#define NUM_PINS 16 // For pin state array

const String code_version = "v0.4.0";

const char* ssid     = "LAN2.4";
const char* password = "skookumchuck.10.18.2019";

int pinState[NUM_PINS]; // TODO consider struct type instead of int

WiFiServer server(80);

// Variable to store the HTTP request
String header;

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0;
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

const long intervalMS = 60000 * 5; // "on" pins will remain on for this long in milliseconds

void setup() {
  Serial.begin(115200);
  // Initialize the output variables as outputs
  // Should be able to use pins 0-5, pins 6-11 are for connecting flash memory chip. 12-16 should also be free
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(PIN4, OUTPUT);
  pinMode(5, OUTPUT);

  analogWriteRange(PWMRANGE);

  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
}

void headerAndStyle(WiFiClient client) {
  client.println("<!DOCTYPE html><html>");
  client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
  client.println("<link rel=\"icon\" href=\"data:,\">");

  client.println("<style>html { font-family: Courier; display: inline-block; margin: 0px auto; text-align: center; background-color: black; color: white; font-size: 20px;}");
  client.println(".buttonOn { font-family: Courier; background-color: #95d159 ; border-style: solid; border-color: white; color: white; margin: 2px; cursor: pointer;}");
  client.println(".buttonOff {background-color: #fc886f;}");
  client.println(".cellOn { color: #3399ff }");
  client.println(".cellOff { color: #000099 }");
  client.println("td, tr {border-bottom: 1px solid #282828}");
  client.println("th {color: #585858}");
  client.println(".numberInput {font-size: 15px}"); // TODO reduce width
  // https://www.w3schools.com/howto/howto_js_rangeslider.asp
  char *sliderCss = "\
  .slidecontainer { \
                    width: 70%;\
                  }\
                    .slider {\
                    -webkit-appearance: none;\
                    appearance: none;\
                    width: 70%; \
                    height: 25px; \
                    background: #d3d3d3; \
                    outline: none; \
                    opacity: 0.7;\
                    -webkit-transition: .2s;\
                    transition: opacity .2s;\
                  }\
                    .slider:hover {\
                    opacity: 1;\
                  }\
                    .slider::-webkit-slider-thumb {\
                    -webkit-appearance: none;\
                    appearance: none;\
                    width: 25px;\
                    height: 25px;\
                    background: #4CAF50;\
                    cursor: pointer; \
                  }\
                    .slider::-moz-range-thumb {\
                    width: 25px;\
                    height: 25px; \
                    background: #4CAF50; \
                    cursor: pointer;\
                  }\
                    ";
  client.println(String(sliderCss));
  client.println("</style><meta charset=\"UTF-8\"></head>");
}

void pinRow(WiFiClient client, int pin) {
  int pinValPWM = readPinState(pin);

  String enableInput = "disabled";
  client.println("<tr>");
  client.println("<td>Pin " + String(pin) + "</td>");

  // on/off buttons
  if (pinValPWM == 0) {
    client.println("<td><a href=\"/" + String(pin) + "/on\"><button id=\"button"+ String(pin) +"\" class=\"buttonOn\">ON</button></a></td>");
  } else {
    enableInput = ""; // TODO recolor input form
    client.println("<td><a href=\"/" + String(pin) + "/off\"><button id=\"button" + String(pin) +"\" class=\"buttonOn buttonOff\">OFF</button></a></td>");
  }

  // raw text input
  client.println("<td id=\"pwmtd" + String(pin) + "\">");
  client.println("<form action=\"/\">");
  client.println("<input id=\"numInput" + String(pin) + "\" class=\"numberInput\"" + enableInput + " type=\"number\" name=\"pwm" + String(pin) + "\" min=\"0\" max=\"" + String(MAX_PWM) + "\" step=\"1\" value=\"" + String(pinValPWM) + "\">");
  client.println("</form>");
  client.println("</td>");

  // slider
  client.println("<td>");
  client.println("<div class=\"slidecontainer\">");
  client.println("<input type = \"range\" min=\"0\" max=\"" + String(MAX_PWM) + "\" value=\"" + String(pinValPWM) + "\" class=\"slider\" id=\"range" + String(pin) + "\">");
  client.println("</div>");
  client.println("</td>");
  client.println("</tr>");
}

void setPinState(int pin, int pwmVal) {
  pinState[pin] = pwmVal;
  analogWrite(pin, pwmVal);
}

int readPinState(int pin) {
  return pinState[pin];
}

void sliderJS(WiFiClient client, int pin) {
  String pinStr = String(pin);
  String sliderName = "slider" + pinStr;
  String inputBoxName = "inputBox" + pinStr;
  String buttonName = "button" + pinStr;

  client.println("<script>");
  client.println("var " + sliderName + " = document.getElementById(\"range" + pinStr + "\");");
  client.println("var " + inputBoxName + " = document.getElementById(\"numInput" + pinStr + "\");");
  client.println("var " + buttonName + " = document.getElementById(\"" + buttonName + "\")");
  client.println(inputBoxName + ".value = " + sliderName + ".value;");
  client.println(sliderName + ".oninput = function() {" + inputBoxName + ".value = this.value; ");
  client.println(inputBoxName + ".disabled = false;");
  client.println("}");
  client.println("</script>");

}

void switcher(String header, int pin) {
  Serial.println(header);

  String pinStr = String(pin);
  bool isPWMSet = header.indexOf("?pwm" + pinStr) >= 0 && header.indexOf("?pwm" + pinStr) < 100; // avoid 'referer' in header from triggering this


  if (header.indexOf("GET /" + pinStr + "/on") >= 0) {
    setPinState(pin, MAX_PWM);
  } else if (header.indexOf("GET /" + pinStr + "/off") >= 0) {
    setPinState(pin, MIN_PWM);
  }

  // Duty cycle input by pin
  if (isPWMSet) {
    const char* headerCStr = header.c_str();
    char* headDup =  strdup(headerCStr); // cannot strtok on a const char*
    char* token = strtok(headDup, "="); // first token is what comes before the '='
    token = strtok(NULL, " "); // Not really safe
    int pwmInt = atoi(token);

    Serial.println("Setting pin " + pinStr + " to " + String(pwmInt));
    setPinState(pin, pwmInt);
  }
}

void pinDebug(int pin, int meta) {
  int analogReadVal = readPinState(pin);
  Serial.println(String(meta) + " ====> " + String(pin) + ": a(" + String(analogReadVal) + ")");
}

void  pinStateDebug() {
  for (int i = 0; i < NUM_PINS ; i++) {
    Serial.println(String(i) + ": + " + String(pinState[i]));
  }
}

void timerJS(WiFiClient client, int pin) {
  int pinVal = readPinState(pin);
  if (pinVal > 0) {
    client.println("<script>");
    client.println("var countDownDate = new Date(new Date().getTime() + " + String(intervalMS) + ").getTime();");
    client.println("var x = setInterval(function() {");
    client.println("var now = new Date().getTime();");
    client.println("var distance = countDownDate - now;");
    client.println("var minutes = Math.floor((distance % (1000 * 60 * 60)) / (1000 * 60));");
    client.println("var seconds = Math.floor((distance % (1000 * 60)) / 1000);");
    client.println("document.getElementById(\"timer" + String(pin) + "\").innerHTML = minutes + \"m \" + seconds + \"s \";");
    client.println("if (distance < 0) { clearInterval(x); document.getElementById(\"timer" + String(pin) + "\").innerHTML = \"EXPIRED\";}},1000)");
    client.println("</script>");
  }
}

void loop() {
  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {                             // If a new client connects,
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    currentTime = millis();
    previousTime = currentTime;
    while (client.connected() && currentTime - previousTime <= timeoutTime) { // loop while the client's connected
      currentTime = millis();
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            switcher(header, 2);
            switcher(header, 3);
            switcher(header, PIN4);
            switcher(header, 5);

            // Display the HTML web page
            headerAndStyle(client);

            // TODO link style none
            client.println("<body><h1>&#x1F480 <a href=\"/\">SKULLFRAKERY " + code_version + "</a> &#x1F480</h1>");

            // Pin status table
            client.println("<table style='width:100%'>");
            client.println("<tr>");
            client.println("<th>Thing</th>");
            client.println("<th>Toggle</th>");
            client.println("<th>Duty Cycle</th>");
            client.println("<th></th>");
            client.println("</tr>");
            pinRow(client, 2);
            pinRow(client, 3);
            pinRow(client, PIN4);
            pinRow(client, 5);
            client.println("</table>");

            sliderJS(client, 2);
            sliderJS(client, 3);
            sliderJS(client, PIN4);
            sliderJS(client, 5);
            client.println("</body></html>");

            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}
