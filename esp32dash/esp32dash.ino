/*
 Name:              esp32dash.ino
 Created:           6/21/2018
 Target hardware:   ESP32
 Author:            0megaIT
 License:			MIT
 Status:            Ready to use
*/

#include <SPIFFS.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>			//External library, install it from https://github.com/bbx10/WebServer_tng
#include <FS.h>
#include <driver/adc.h>
#include <ESPmDNS.h>			//mDNS settings by default: http://myESP32.local open esp32dash page (for the Windows OS under v.10, use a third-party, e.g. Apple Bonjour)

#define WIFI_CLIENT_DHCP 1		//1 - DHCP mode ON, 0 - DHCP mode OFF

#define BUILTIN_LED_PIN 2
#define WIFI_TRY_COUNT 30
#define MAX_DASH_ITEMS_COUNT 30

#define WIFI 0
#define MILLIS 1
#define HALL 3
//<----- define your types here

WebServer server(80);
const char* host = "myESP32";

int wifiIP[5];
int wifiNetMask[5];
int wifiGateway[5];

char wifiSSID[33];
char wifiPASS[65];

typedef struct
{
	char name[20];
	unsigned short iconUnicode;
	unsigned short type;
	double value;
} DashItems;

DashItems dashItem[MAX_DASH_ITEMS_COUNT];
unsigned short dashItemsCount;

bool esp32Started;

// the setup function runs once when you press reset or power the board
void setup() 
{
	esp32Started = false;

	Serial.begin(115200);
	Serial.println("\n\n\n---> ESP starting begin<---");

	if (!WiFiInit())
		return;

	if (!FSInit())
		return;

	if (!ServerInit())
		return;

	if (!DashItemsInit())
		return;

	esp32Started = true;

	Serial.println("\n\n\n---> Done: ESP32 Started <---");
	Serial.println("");
	Serial.println("All is ready!");
	Serial.println("Go to the esp32dash in a web-browser on your pc or mobile:");
	Serial.print("http://"); Serial.println(WiFi.localIP()); 
	Serial.println("or");
	Serial.printf("http://%s.local \n", host); 
	Serial.println("");
}

// the loop function runs over and over again until power down or reset
void loop() 
{

	//Update data

	//WiFi signal level
	dashItem[0].value = getWifiSignalLevel();
	
	//Millis
	dashItem[1].value = millis();

	//Built in Hall sensor
	dashItem[2].value = hallRead();

	//handle server requests
	server.handleClient();

	delay(1);
}


//----------------------------------------------------------- WI-FI INIT -----------------------------------------
bool WiFiInit()
{
	strcpy(wifiSSID, "OMEGA");					//<-----Set your Wi-Fi network SSID
	strcpy(wifiPASS, "essy123essy");			//<-----Set your Wi-Fi network PASS

	Serial.println("Enabling Wi-Fi...");

	WiFi.onEvent(WiFiEvent);
	
	if (!WIFI_CLIENT_DHCP)						///#define WIFI_CLIENT_DHCP 0 to disable DHCP mode and set IP's manually
	{
		IPAddress local_IP(192, 168, 1, 117);	//<-----Set any unoccupied IP 
		IPAddress gateway(192, 168, 1, 1);		//<-----Set your network gateway
		IPAddress subnet(255, 255, 255, 0);		//<-----Set your network subnet mask

		IPAddress dns(8, 8, 8, 8);				//Google Public DNS

		Serial.println("Network settings: ");

		local_IP = IPAddress(local_IP[0], local_IP[1], local_IP[2], local_IP[3]);
		Serial.print("IP: ");
		Serial.println(local_IP);

		gateway = IPAddress(gateway[0], gateway[1], gateway[2], gateway[3]);
		Serial.print("Gateway: ");
		Serial.println(gateway);

		subnet = IPAddress(subnet[0], subnet[1], subnet[2], subnet[3]);
		Serial.print("Subnet: ");
		Serial.println(subnet);

		WiFi.config(local_IP, gateway, subnet, dns);
	}
	
	Serial.printf("Connecting to %s\n", wifiSSID);

	WiFi.begin(wifiSSID, wifiPASS);

	for (int i = 0; i < WIFI_TRY_COUNT; i++)
	{
		Serial.print(".");
		delay(500);
		if (WiFi.status() == WL_CONNECTED)
		{
			Serial.println("\nConnected!");
			break;
		}
	}

	if (WiFi.status() == WL_CONNECTED)
	{
		if (MDNS.begin(host))
		{
			Serial.println("mDNS responder started.");
			Serial.println("(For the Windows OS under v.10, use a third-party, e.g. Apple Bonjour.)");

		}

		Serial.println("---> Done: Wi-Fi init <---");
		BlinkWiFiClientStarted();
		return true;
	}
	else
	{
		Serial.println("Can't connect as a Wi-Fi client!");
		BlinkWiFiClientFailed();
		return false;
	}
}

void BlinkWiFiClientStarted()
{
	if (BUILTIN_LED_PIN >= 0)
	{
		pinMode(BUILTIN_LED_PIN, OUTPUT);

		for (int i = 0; i < 2; i++)
		{
			digitalWrite(BUILTIN_LED_PIN, HIGH);
			delay(500);
			digitalWrite(BUILTIN_LED_PIN, LOW);
			delay(500);
		}
	}
}

void BlinkWiFiClientFailed()
{
	if (BUILTIN_LED_PIN >= 0)
	{
		pinMode(BUILTIN_LED_PIN, OUTPUT);

		for (int i = 0; i < 5; i++)
		{
			digitalWrite(BUILTIN_LED_PIN, HIGH);
			delay(100);
			digitalWrite(BUILTIN_LED_PIN, LOW);
			delay(100);
		}
	}
}

void WiFiEvent(WiFiEvent_t event)
{
	if (event == SYSTEM_EVENT_STA_GOT_IP)
	{
		Serial.printf("\nGot IP: %s\r\n", WiFi.localIP().toString().c_str());
	}
	else if (event == SYSTEM_EVENT_STA_DISCONNECTED)
	{
		Serial.printf("\nDisconnected from Wi-Fi!\n");
	}
	else
	{
		Serial.printf("\nWi-Fi event! Code: %d\r\n", event);
	}
}

int getWifiSignalLevel()
{
	int dBm, quality;
	dBm = WiFi.RSSI();
	if (dBm <= -100)
		quality = 0;
	else if (dBm >= -50)
		quality = 100;
	else
		quality = 2 * (dBm + 100);
	return quality;
}
//----------------------------------------------------------- WI-FI INIT -----------------------------------------

//----------------------------------------------------------- HANDLE FILE SYSTEM ---------------------------------
bool FSInit()
{
	if (SPIFFS.begin(false, "/spiffs", 100) == true)
	{
		Serial.println("---> Done: FileSystem inint <---");
		return true;
	}
	else
	{
		Serial.println("---> Failed: FileSystem inint <---");
		return false;
	}
}
//----------------------------------------------------------- HANDLE FILE SYSTEM ---------------------------------

//----------------------------------------------------------- HANDLE HTTP SERVER REQ -----------------------------
String getContentType(String filename) 
{
	if (server.hasArg("download")) return "application/octet-stream";
	else if (filename.endsWith(".htm")) return "text/html";
	return "text/plain";
}

bool handleFileRead(String path) 
{
	Serial.println("handleFileRead: " + path);
	if (path.endsWith("/")) path += "index.htm";
	String contentType = getContentType(path);
	if (SPIFFS.exists(path)) 
	{
		File file = SPIFFS.open(path, "r");
		size_t sent = server.streamFile(file, contentType);
		file.close();
		return true;
	}
	return false;
}

bool ServerInit()
{
	//SERVER INIT HTTP
	server.onNotFound([]() 
	{
		if (!handleFileRead(server.uri()))
			server.send(404, "text/plain", "FileNotFound");
	});
	server.on("/", HTTP_GET, [](){if (!handleFileRead("/"))server.send(404, "text/plain", "FileNotFound");});  //open home page
																											   //get heap status, analog input value and all GPIO statuses in one json call
	server.on("/items", HTTP_GET, []()
	{
		String json = "[";
		for(int i = 0; i < dashItemsCount; i++)
		{
			json += "{\"name\":\"" + String(dashItem[i].name) + "\", \"type\":" + String(dashItem[i].type) + ", \"iconUnicode\":" + String(dashItem[i].iconUnicode) + +", \"value\":" + String(dashItem[i].value) + "}";
			if (i < dashItemsCount - 1)
				json +=  ",";
		}
		json += "]";

		server.send(200, "text/json", json);
		json = String();
	}); 
	
	server.begin();

	Serial.println("---> Done: HTTP server started <---");

	return true;
}
//----------------------------------------------------------- HANDLE HTTP SERVER REQ -----------------------------

//----------------------------------------------------------- HANDLE DASHBOARD ITEMS -----------------------------
bool DashItemsInit()
{
	dashItemsCount = 0;

	//WiFi signal level
	dashItem[dashItemsCount].type = WIFI;
	strcpy(dashItem[dashItemsCount].name, "Wi-Fi");
	dashItem[dashItemsCount].iconUnicode = 61931;
	dashItem[dashItemsCount].value = getWifiSignalLevel();
	dashItemsCount++;

	//Millis
	dashItem[dashItemsCount].type = MILLIS;
	strcpy(dashItem[dashItemsCount].name, "Millis");
	dashItem[dashItemsCount].iconUnicode = 61463;
	dashItem[dashItemsCount].value = millis();
	dashItemsCount++;

	//Built in Hall sensor as a sample
	dashItem[dashItemsCount].type = HALL;
	strcpy(dashItem[dashItemsCount].name, "Hall");
	dashItem[dashItemsCount].iconUnicode = 61558;
	adc1_config_width(ADC_WIDTH_12Bit);
	dashItem[dashItemsCount].value = hallRead();
	dashItemsCount++;

	//.....
	//<----- Add your dashboard items initialization here (sensors, variables, output states and so on) 
	//.....

	Serial.println("---> Done: Dash Items init <---");

	return true;
}
//----------------------------------------------------------- HANDLE DASHBOARD ITEMS -----------------------------
