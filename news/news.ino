#include <SPI.h>
#include "LedMatrix.h"
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#define ssid  "xx"
#define password "$xxx!23"

#define NUMBER_OF_DEVICES 5
#define CS_PIN D4

#define MAX_FEEDS 5

unsigned long int newsCounter = 0;

// Thinger - uRn1z77&cGpT

bool resetFromWeb = false;

ESP8266WebServer server;

LedMatrix ledMatrix = LedMatrix(NUMBER_OF_DEVICES, CS_PIN);

unsigned long tRefreshRSSFeed = 0, tLEDMatrixInterval = 0;

const String rssHostList[] = 
{
	"http://www.dnaindia.com/feeds/india.xml",
	"http://timesofindia.indiatimes.com/rssfeedstopstories.cms",
	"http://www.thehindubusinessline.com/news/?service=rss",
	"http://feeds.reuters.com/reuters/INtopNews?format=xml",
	"http://indianexpress.com/section/world/feed/"
};

//const char *rssHost = "www.dnaindia.com";
//const char *rssFeedXML = "/feeds/scitech.xml";

//const char *rssHost = "timesofindia.indiatimes.com";
//const char *rssFeedXML = "/rssfeedstopstories.cms";

//const char *rssHost = "www.thehindubusinessline.com";
//const char *rssFeedXML = "/news/?service=rss";

//const char *rssHost = "open.live.bbc.co.uk";
//const char *rssFeedXML = "/weather/feeds/en/2643123/3dayforecast.rss";

const int rssPort = 80;

String localIP = "";

bool multiLineFeed = false; 
  
void setup() {
	Serial.begin(115200);

	tRefreshRSSFeed = tLEDMatrixInterval = millis();

	resetFromWeb = false;

	ledMatrix.init();
	ledMatrix.setText("Welcome to NowNews ! ... connecting ...");

	if(connectToWiFi()) {
		String str = getTitlesFromFeed(getRSSFeedAsRawXML("timesofindia.indiatimes.com", "/rssfeedstopstories.cms"), 10);
		Serial.println(str);

		ledMatrix.setNextText("Welcome to IDC Platform Dev Bay .... NEWS: " + str);
	}
}

void loop() {
	server.handleClient();

	if(millis() - tLEDMatrixInterval > 0) { 
		ledMatrix.clear();
		ledMatrix.scrollTextLeft();
		ledMatrix.drawText();
		ledMatrix.commit();

		tLEDMatrixInterval = millis();
	}

	if((millis() - tRefreshRSSFeed > 200000) || resetFromWeb) {
		// get 10 stories
		String rssHostFeed = rssHostList[newsCounter++%MAX_FEEDS];
		String host = "";
		String feed = "";

		int indexHost1 = rssHostFeed.indexOf("http://");
		host = rssHostFeed.substring(7);
		int indexHost2 = host.indexOf("/");
		
		feed = host.substring(indexHost2);
		host = host.substring(0, indexHost2);

		Serial.print("Host : ");
		Serial.println(host);

		Serial.print("Feed : ");
		Serial.println(feed);

		int hostLength = host.length();
		char hostAsChar[hostLength+1];
		host.toCharArray(hostAsChar, hostLength+1);

		String newsFeed = getTitlesFromFeed(getRSSFeedAsRawXML(hostAsChar, feed), 10);

		Serial.print("Feed : ");
		Serial.println(newsFeed);

		ledMatrix.setNextText("Welcome to IDC Platform .. NEWS [" + host + "]:" + newsFeed);

		tRefreshRSSFeed = millis();
		resetFromWeb = false;
	}
}

// connects to Wifi and sets up the WebServer !
bool connectToWiFi() {
	Serial.println();
	Serial.println();
	Serial.print("Connecting to ");
	Serial.println(ssid);

	int count = 0;

	WiFi.begin(ssid, password);

	while (WiFi.status() != WL_CONNECTED) {
		if(count++ > 30) // 30 attempts
			return false; 
		delay(500);
		Serial.print(".");
	}

	Serial.println("");
	Serial.println("WiFi connected");
	Serial.println("IP address: ");
	
	localIP = WiFi.localIP().toString();

	Serial.println(localIP);

	// run the server now.
	server.on("/", [](){server.send(200, "text/plain", "Welcome to Now News!");});
	server.on("/reset", []() {
		resetFromWeb = true;
		server.send(200, "text/plain", "Reset is done !\n");
	});  

	// start the server
	Serial.println("Starting the server ... ");
	server.begin(); 

	return true;
}

// extract the titles from the RSS feed
String getTitlesFromFeed(String feed, int maxStories) 
{
	int totalStories = 0; 
	String item;

	String newsStream = "";

	if(feed == "")
		return "";

	int itemIndex1 = 0, itemIndex2 = 0, titleIndex1 = 0, titleIndex2 = 0;

	while(++totalStories < maxStories) {

		if(!multiLineFeed) {
			itemIndex1 = feed.indexOf("<item>");
			itemIndex2 = feed.indexOf("</item>");

			if(itemIndex1 < 0 || itemIndex2 < 0) break;

			// get content within <item> ... </item> first
			item = feed.substring(itemIndex1+6, itemIndex2); 
		} else {
			item = feed;
		}


		// get content within <title> ... </title> within the item
		titleIndex1 = item.indexOf("<title>");
		titleIndex2 = item.indexOf("</title>");

		if(titleIndex1 < 0 || titleIndex2 < 0) break;

		String text = item.substring(titleIndex1+7, titleIndex2);

		Serial.print("Title -- >> ");
		Serial.println(text);
		
		// For specific RSS feeds
		// its possible that its embedded within ![CDATA[ to prevent 
		// browser exec of scripts, strip it down 
		if(text.indexOf("<![CDATA[") >= 0) { 
			text.replace("<![CDATA[", "");
			text.replace("]]>", "");
			// remove possible newlines too
			//text.replace("\n", "");
		}

		if(!multiLineFeed)
			feed = feed.substring(itemIndex2+7);
		else
			feed = feed.substring(titleIndex2+8);

		if(newsStream == "") // first item;
			newsStream = text;
		else 
			newsStream = newsStream + " ... " + text;

		//Serial.println(text);		
	}

	return newsStream;
}

String getRSSFeedAsRawXML(const char* rssHost, String rssURL) {

	Serial.print("Trying to connect to ");
	Serial.println(rssHost);

	// Use WiFiClient 
	WiFiClient client;
	if (!client.connect(rssHost, rssPort)) {
		Serial.print("Failed to connect to :");
		Serial.println(rssHost);
		return "";
	}

	Serial.println("connected !....");

	client.print(String("GET ") + rssURL + " HTTP/1.1\r\n" +
	           "Host: " + rssHost + "\r\n" +
	           "User-Agent: ESP8266\r\n" +
	           "Content-Type: application/xml\r\n" +
	           "Accept: */*\r\n" +
	           "Connection: close\r\n\r\n");

	// bypass HTTP headers
	while (client.connected()) {
		String line = client.readStringUntil('\n');
		//Serial.println( "Header: " + line );
		if (line == "\r") {
		  break;
		}
	}

	String rssFeed = "";
	bool startReadingItem = false;
	bool startReadingTitle = false;

	while (client.connected()) {
		String line = client.readStringUntil('\n');

		//Serial.print("Line -- ");

		if(line.indexOf("<item>") >= 0)
			startReadingItem = true; 

		if(startReadingItem)
		{
			if(line.indexOf("<title>") >= 0)
				startReadingTitle = true;

			if(startReadingTitle) { 
				//Serial.print("Line -- ");
				//Serial.println(line);
				rssFeed = rssFeed + line;
			}
			
			if(line.indexOf("</title>") >= 0)
				startReadingTitle = false;
		}

		if(line.indexOf("</item>") >= 0)
			startReadingItem = false; 		
				
		if(line.indexOf("</rss>") >= 0)
			break;

		if(!multiLineFeed) multiLineFeed = true;
	}

	return rssFeed;
}