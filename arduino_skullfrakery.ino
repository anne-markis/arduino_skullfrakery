#include <ESP8266WiFi.h>

const String code_version = "v0.1.0";

const char* ssid     = "LAN2.4";
const char* password = "skookumchuck.10.18.2019";

WiFiServer server(80);

// Variable to store the HTTP request
String header;

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

const long intervalMS = 60000; // "on" pins will remain on for this long in milliseconds
//unsigned long previousMillis = 0;

void setup() {
  Serial.begin(115200);
  // Initialize the output variables as outputs
  // Should be able to use pins 0-5, pins 6-11 are for connecting flash memory chip. 12-16 should also be free
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);

  // Set outputs to LOW
  digitalWrite(2, LOW);
  digitalWrite(3, LOW);
  digitalWrite(4, LOW);
  digitalWrite(5, LOW);

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
  client.println("</style><meta charset=\"UTF-8\"></head>");
}

void pinRow(WiFiClient client, int pin) {
  int pinVal = digitalRead(pin);
  client.println("<tr>");
  client.println("<td>Pin " + String(pin) + "</td>");

  if (pinVal == LOW) {
    client.println("<td class=\"cellOff\"> LOW </td>");
    client.println("<td><a href=\"/"+String(pin)+"/on\"><button class=\"buttonOn\">ON</button></a></td>");
    client.println("<td id=\"timer\">-</td>"); // TODO dynamtic id name
  } else {
    client.println("<td class=\"cellOn\"> HIGH </td>");
    client.println("<td><a href=\"/"+String(pin)+"/off\"><button class=\"buttonOn buttonOff\">OFF</button></a></td>");
    client.println("<td id=\"timer\">"+String(intervalMS)+" ms</td>");
  }
  client.println("</tr>");
}

void switcher(String header, int pin) {
   String pinStr = String(pin);
   if (header.indexOf("GET /"+pinStr+"/on") >= 0) {
      Serial.println("Turning pin " + pinStr + " on");
      digitalWrite(pin, HIGH);
    } else if (header.indexOf("GET /"+pinStr+"/off") >= 0) {
      Serial.println("Turning pin " + pinStr + " off");
      digitalWrite(pin, LOW);
    }
}

void timerJS(WiFiClient client) {
  client.println("<script>");
  client.println("var countDownDate = new Date(\"Jan 5, 2021 15:37:25\").getTime();");

// Update the count down every 1 second
  client.println("var x = setInterval(function() {");

  // Get today's date and time
  client.println("var now = new Date().getTime();");

  // Find the distance between now and the count down date
  client.println("var distance = countDownDate - now;");

  // Time calculations for days, hours, minutes and seconds
  client.println("var days = Math.floor(distance / (1000 * 60 * 60 * 24));");
  client.println("var hours = Math.floor((distance % (1000 * 60 * 60 * 24)) / (1000 * 60 * 60));");
  client.println("var minutes = Math.floor((distance % (1000 * 60 * 60)) / (1000 * 60));");
  client.println("var seconds = Math.floor((distance % (1000 * 60)) / 1000);");

  // Display the result in the element with id="demo"
  client.println("document.getElementById(\"timer\").innerHTML = days + \"d \" + hours + \"h \" + minutes + \"m \" + seconds + \"s \";");

  // If the count down is finished, write some text
  client.println("if (distance < 0) { clearInterval(x); document.getElementById(\"demo\").innerHTML = \"EXPIRED\";}},1000)");
  client.println("</script>");
}

void loop(){
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
            switcher(header, 4);
            switcher(header, 5);
              
            // Display the HTML web page
            headerAndStyle(client);
            
            client.println("<body><h1>&#x1F480 SKULLFRAKERY " + code_version + " &#x1F480</h1>");
            
            // Pin status table
            client.println("<table style='width:100%'>");
            client.println("<tr>");
            client.println("<th>Thing</th>");
            client.println("<th>Value</th>");
            client.println("<th>Toggle</th>");
            client.println("<th>Timer</th>");
            client.println("</tr>");
            client.println("<tr>");
            client.println("<td>A0</td>");
            client.println("<td>" + String(analogRead(A0)) + "v</td>");
            client.println("<td></td>");
            client.println("<td></td>");
            client.println("</tr>");
            pinRow(client, 2);
            pinRow(client, 3);
            pinRow(client, 4);
            pinRow(client, 5);
            client.println("</table>");
            timerJS(client); // TODO
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
