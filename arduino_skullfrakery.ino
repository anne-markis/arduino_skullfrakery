/*********
  Rui Santos
  Complete project details at https://randomnerdtutorials.com  
*********/

#include <ESP8266WiFi.h>

const String code_version = "v0.0.1";

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
  // CSS to style the on/off buttons 
  // Feel free to change the background-color and font-size attributes to fit your preferences
  client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
  client.println(".buttonOn { background-color: #DAF7A6 ; border: none; color: white; padding: 15px 40px;");
  client.println("text-decoration: none; font-size: 15px; margin: 2px; cursor: pointer;}");
  client.println(".buttonOff {background-color: #fc886f;}</style></head>");
}

void pinRow(WiFiClient client, int pin) {
  int pinVal = digitalRead(pin);
  client.println("<tr>");
  client.println("<td>Pin " + String(pin) + "</td>");
  client.println("<td>" + String(pinVal) + "</td>");
    if (pinVal==LOW) {
    client.println("<td><a href=\"/"+String(pin)+"/on\"><button class=\"buttonOn\">ON</button></a></td>");
  } else {
    client.println("<td><a href=\"/"+String(pin)+"/off\"><button class=\"buttonOn buttonOff\">OFF</button></a></td>");
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

//            switcher(header, 1);  // doesnt work
            switcher(header, 2);
            switcher(header, 3);
            switcher(header, 4);
            switcher(header, 5);
              
            // Display the HTML web page
            headerAndStyle(client);
            client.println("<body><h1>SKULLFRAKERY " + code_version + "</h1>");
            
            // Pin status table
            client.println("<table style='width:100%'>");
            client.println("<tr>");
            client.println("<th>Thing</th>");
            client.println("<th>Value</th>");
            client.println("<th>Toggle</th>"); // TBD
            client.println("</tr>");
            client.println("<tr>");
            client.println("<td>A0</td>");
            client.println("<td>" + String(analogRead(A0)) + "</td>");
            client.println("<td>...</td>");
            client.println("</tr>");
            pinRow(client, 2);
            pinRow(client, 3);
            pinRow(client, 4);
            pinRow(client, 5);
            client.println("</table>");

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
