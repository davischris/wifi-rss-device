#include "WiFi.h"
#include <U8x8lib.h>
#include <ArduinoJson.h>
#include <math.h>

// the OLED used
U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(/* clock=*/ 15, /* data=*/ 4, /* reset=*/ 16);

const char* host = "square.famousdavis.net";
const size_t MAX_CONTENT_SIZE = 512;       // max size of the HTTP response
const unsigned long HTTP_TIMEOUT = 100000;  // max respone time from server

// character dimensions (for aligning text)
#define CH_WID 6
#define CH_HEI 8
// display dimensions (for aligning text)
#define DP_WID 128
#define DP_HEI 64
#define DP_WID_MID 64
#define DP_HEI_MID 32

//#define OLED_RESET LED_BUILTIN //4
//Adafruit_SSD1306 display(OLED_RESET);

#define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define DELTAY 2

// The type of data that we want to extract from the page
struct SalesData {
  char totalCollected[32];
  char grossSales[32];
  char netSales[32];
  char transactionCount[32];
  char lastWeek[32];
  char sixMonths[32];
  char lastTransactionTime[32];
};

#define LOGO16_GLCD_HEIGHT 16 
#define LOGO16_GLCD_WIDTH  16 

//#if (SSD1306_LCDHEIGHT != 64)
//#error("Height incorrect, please fix Adafruit_SSD1306.h!");
//#endif

//const char* ssid = "Peace Love Goodness";
//const char* password = "aaaaaaaaaa";
const char* ssid = "iPhone";
const char* password = "aaaaaaaaab";

WiFiClient client;

void setup()
{
  // Set WiFi to station mode and disconnect from an AP if it was previously connected
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  u8x8.begin();
  u8x8.setFont(u8x8_font_chroma48medium8_r);

  // Connect to WiFi network
   u8x8.clearDisplay();
   u8x8.drawString(0, 0, "Connecting to ");
   u8x8.drawString(0,1,ssid);
  WiFi.disconnect(true);
  
  WiFi.begin(ssid, password);
  u8x8.setCursor(0,4);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    u8x8.print(".");
  }
  u8x8.clearDisplay();
  u8x8.drawString(0,2,"WiFi connected");
  
}


static void doSomeWork()
{
  int n = WiFi.scanNetworks();

  if (n == 0) {
    u8x8.drawString(0, 0, "Searching networks.");
  } else {
    u8x8.drawString(0, 0, "Networks found: ");
    for (int i = 0; i < n; ++i) {
      // Print SSID for each network found
      char currentSSID[64];
      WiFi.SSID(i).toCharArray(currentSSID, 64);
      u8x8.drawString(0, i + 1, currentSSID);
    }
  }

  // Wait a bit before scanning again
  delay(5000);
}


void loop()
{
  //doSomeWork();
  SalesData salesData;
  
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    u8x8.print("connection failed");
    return;
  }

  const size_t BUFFER_SIZE =
      JSON_OBJECT_SIZE(7)    // the root object has 8 elements
      + MAX_CONTENT_SIZE;    // additional space for strings

  DynamicJsonBuffer jsonBuffer(BUFFER_SIZE);
         
  // We now create a URI for the request
  String url = "/getsales.php?data";
  
  // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" + 
               "Connection: close\r\n\r\n");
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > HTTP_TIMEOUT) {
      u8x8.print(">>> Client Timeout !");
      client.stop();
      return;
    }
  }

  skipResponseHeaders();
  JsonObject& root = jsonBuffer.parseObject(client);
  if (!root.success()) {
    u8x8.print("JSON parsing failed!");
    delay(5000);
  }

  char buf[30];
  
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  u8x8.clearDisplay();
  strcpy(salesData.grossSales, root["grossSales"]);
  strcpy(salesData.lastWeek, root["lastWeek"]);
  strcpy(salesData.sixMonths, root["sixMonths"]);
  //display.setCursor(DP_WID_MID - (3 * CH_WID * 2),15);
  //display.setTextSize(2);
  u8x8.draw2x2String(0,2,"$");
  u8x8.draw2x2String(2,2,salesData.grossSales);
  u8x8.drawString(0,4,"1 Wk: ");
  if(atof(salesData.lastWeek) > 0) {
    u8x8.drawString(6,4,"+");
  }
  String a = (String)round(atof(salesData.lastWeek));
  char b[a.length()+1];
  a.toCharArray(b,a.length()+1);
  strcpy(buf,b);
  strcat(buf,"%");
  u8x8.drawString(7,4,salesData.lastWeek);
  u8x8.drawString(0,5,"6 Mo: ");
  if(atof(salesData.sixMonths) > 0) {
     u8x8.drawString(6,5,"+");
  }
  strcpy(buf,salesData.sixMonths);
  strcat(buf,"%");
  u8x8.drawString(7,5,salesData.sixMonths);

  delay(60000);
}

// Skip HTTP headers so that we are at the beginning of the response's body
bool skipResponseHeaders() {
  // HTTP headers end with an empty line
  char endOfHeaders[] = "\r\n\r\n";

  client.setTimeout(HTTP_TIMEOUT);
  bool ok = client.find(endOfHeaders);

  if (!ok) {
    u8x8.print("No response or invalid response!");
  }

  return ok;
}
