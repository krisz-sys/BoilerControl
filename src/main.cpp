#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ESP_EEPROM.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include <String.h>
#include "DHT.h"
#include <ArduinoOTA.h>
#include <ESPAsyncWebServer.h>
#include <ESP8266mDNS.h>
#include <time.h>

#include "index.h"
// -------------------------------------- time.h
const char* TZ_INFO = "EET-2EEST,M3.5.0,M10.5.0/3";

// -------------------------------------- NTP server

String formattedDate;
String dayStamp;
String timeStamp;
long date_check_timer;
//--------------------------------------  UDP communication
unsigned int localUdpPort = 9800;
char incomingPacket[255];  // buffer for incoming packets
char respond[255];
char  replyPacket[] = "Hi there! Got the message :-)";  // a reply string to send back
WiFiUDP Udp;
//char* remoteIp="";
String remoteIp="";
//--------------------------------------- temp, humidity 
#define DHTPIN D7              // GPIO13
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
float humidity=-10;
float temperature=-10;
// --------------------------------------- comm protocol

#define COMM_ERR 56
#define TEMP_REQUEST 39
#define TEMP_FLAG 40
#define HUM_REQUEST 41
#define HUM_FLAG 42
#define TEMP_SENSOR_ERROR 55
#define MSG_RECEIVED 65
//#define SET_LED 66
#define HEATER_ON 60                                  // turn ON theheater for x minutes
#define HEATER_REQUEST 61                             
#define HEATER_REMAINING 62                           // remaining time when the heater is ON
#define HUM_SENSOR_ERR 101                // the humidity sensor is damaged or don t works
#define BOSS 50
#define ADD_TSCH 51
#define ADD_TSCH_CONF 52
#define REMOVE_TSCH 53
#define REMOVE_TSCH_CONF 54
#define REMOVE_TSCH_NCONF 59
#define GET_TSCHS 57
#define SEND_TSCHS 58



//---------------------------------------- Heater
#define HEATER D5                             // GPIO14
//#define HEATER D1                             // GPIO05
bool ledSPMode=true;                       // false => led SP from the POT,       true => led SP from the sender
int lastSample=0;
bool firstStart=true;

//----------------------------------------wifi

//const char *ssid = "BV";          //Enter your wifi SSID
const char *ssid = "yourSSID";
const char *password = "yourPassword";        //Enter your wifi Password
IPAddress local_IP(192, 168, 1, 223);       // a telefonba ezt az ip-t kell beírni
//IPAddress local_IP(192, 168, 1, 223);
IPAddress gateway(192, 168, 1,1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);           //optional
IPAddress secondaryDNS(8, 8, 4, 4);

#define debug(x) Serial.print(x)
#define debugln(x) Serial.println(x)

long reconnect_period=120*1000;
long reconnectTimer;
long universalTimer;                        // call the other functionalitys every 100 ms

long testTimer;

long heaterCounter=0;                           // counts down every 3000ms until 0
long heaterTimer;
long heaterHandlerPeriod=3000;
long tempSamplePeriod=30000;

int TSCH_array[20][3];
int TSCH_len=0;
int currentTime=0;           // current time in minutes
long checkTSCHTimer;
long  tempSampleTimer;

AsyncWebServer server1(80);
AsyncWebSocket ws("/ws");  // WebSocket endpoint

// Function to send value updates to clients
void notifyClients(int heaterVal, float temp) {

  String finalString=String(heaterVal)+", "+String(temperature,1);

  for(int i=0;i<3;++i){
    finalString+=", ";
    int hour=TSCH_array[i][0];
    int min=TSCH_array[i][1];
    int time=TSCH_array[i][2];
    
    if(hour<10){
      finalString+="0";
    }
    finalString+=String(hour)+":";
    if(min<10){
      finalString+="0";
    }
    finalString+=String(min)+", "+String(time);
  }
  
  ws.textAll(finalString);  // send value to all connected clients
}

// Handle incoming WebSocket messages (optional)
void onWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;

  if (info->final && info->index == 0 && info->opcode == WS_TEXT) {

    // Safe copy
    char message[len + 1];
    memcpy(message, data, len);
    message[len] = 0;

    String msg = String(message);
    Serial.println("Received: " + msg);

    // Expecting message like: "Slider: 234"
    String prefix = "Slider: ";
    if (msg.startsWith(prefix)) {
      String sliderVal = msg.substring(prefix.length());

      heaterCounter = sliderVal.toInt();
      if (heaterCounter > 0) {
        digitalWrite(HEATER, HIGH);
      }

      Serial.println("Parsed slider value: " + sliderVal);
    }

    prefix = "Button: ";

    if (msg.startsWith(prefix)) {
      String buttonVal = msg.substring(prefix.length());
      Serial.println("Button value: " + buttonVal);
    }


    
    if(msg.startsWith("TS") && msg.charAt(4)=='T'){
        prefix=msg.substring(0,7);
        int index=int(msg.charAt(2)-'0')-1;

      if (msg.startsWith(prefix)) {
        String TS1_Tval = msg.substring(prefix.length());
        int time=atoi(TS1_Tval.c_str());
        TSCH_array[index][2]=time;
        EEPROM.write(index*3+3, TSCH_array[index][2]);
        EEPROM.commit();
      }
    }
    

    if(msg.startsWith("TS") && msg.charAt(4)=='H'){
      prefix=msg.substring(0,7);
      int index=int(msg.charAt(2)-'0')-1;

      if (msg.startsWith(prefix)) {
        String TS1_Hval = msg.substring(prefix.length());
        String hour=TS1_Hval.substring(0,2);
        String minute=TS1_Hval.substring(3,5);

        TSCH_array[index][0]=atoi(hour.c_str());
        TSCH_array[index][1]=atoi(minute.c_str());
        EEPROM.write(index*3+1, TSCH_array[index][0]);
        EEPROM.write(index*3+2, TSCH_array[index][1]);
        EEPROM.commit();
      }
    }


  }
}

void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, 
                      AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.printf("Client #%u connected\n", client->id());
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.printf("Client #%u disconnected\n", client->id());
  } else if (type == WS_EVT_DATA) {
    onWebSocketMessage(arg, data, len);
  }
}

String sliderValue = "0";
bool startMeasuring=false;


void connect2Wifi(int);
int receiveUDP();
void decodeAndBuildRespond();
void getTime();
int receiveUDP();
void sendUDP();
void readHumAndTemp();
bool temp2bytes(char*);
void turnON(int);
String processor(const String& var);
void temp1Request(AsyncWebServerRequest *request);
void sliderRequest(AsyncWebServerRequest *request);

void setup() {
  EEPROM.begin(512);
  Serial.begin(115200);
  
  dht.begin();
  pinMode(HEATER,OUTPUT);


  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("STA Failed to configure");
  }
  
  Serial.printf("Connecting to %s ", ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password); //Connect to wifi
  //WiFi.begin(ssid); //Connect to wifi
  connect2Wifi(200);

  Udp.begin(localUdpPort);

  reconnectTimer=millis();
  date_check_timer=millis();
  universalTimer=millis();
  heaterTimer=millis();
  checkTSCHTimer=millis();
  testTimer=millis();
  tempSampleTimer=millis();

  
  //TSCH_len=EEPROM.read(0);
  TSCH_len=4;
  for(int i=0;i<TSCH_len;++i){
    TSCH_array[i][0]=EEPROM.read(i*3+1);
    TSCH_array[i][1]=EEPROM.read(i*3+2);
    TSCH_array[i][2]=EEPROM.read(i*3+3);
  }
  // ---------------------------------- webserver

  server1.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html; charset=utf-8",index_html);
  });
  ws.onEvent(onWebSocketEvent);
  server1.addHandler(&ws);
  server1.begin();

    //---------------------------------- OTA

// Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
   ArduinoOTA.setHostname("ESP8266-Boiler controller");

  // No authentication by default
  ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else {  // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();

  if (MDNS.begin("boilercontrol")) {
    Serial.println("mDNS responder started");
  }

  // ------------------ time.h

  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  
  // Időzóna érvényesítése
  setenv("TZ", TZ_INFO, 1);
  tzset();
}


String processor(const String& var){
  //Serial.println(var);
  if (var == "SLIDERVALUE"){
    return sliderValue;
  }
  return String();
}

void temp1Request(AsyncWebServerRequest *request){
      startMeasuring=true;
      //readHumAndTemp();
      request->send(200, "text/plain", String(temperature,1));          // sends with precision 1
}

void sliderRequest(AsyncWebServerRequest *request) {
  request->send(200, "text/plain", String(heaterCounter,0));
    
  }



void checkTSCH(){
  for(int i=0;i<TSCH_len;++i){
    int targetTime=60*((int)TSCH_array[i][0])+(int)TSCH_array[i][1];            // calculating the minutes => hours*60+minutes
    if(targetTime==currentTime){                                                // starting 
      turnON((int)TSCH_array[i][2]);
      debug("match: ");
      debugln(heaterCounter);
    }
  }
}

void heaterHandler(){

  if(heaterCounter>=0){
    heaterCounter--;
  }
  if(heaterCounter<0){
    digitalWrite(HEATER,0);
  }
}

void connect2Wifi(int max_attempts){
  int i=0;
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(50);
    Serial.print(".");
    if(i>=max_attempts){
      break;
    }
    i++;
  }
  debugln("");
  debug("localIP: ");
  debugln(WiFi.localIP());
}


void decodeAndBuildRespond(){
  //debugln("decoding: ");
  for(int i=1;i<(int)incomingPacket[0];++i){
    debug((int)incomingPacket[i]);
    debug(" ");
  }
  //debugln("");
  int i=0;                    // counter of the receiver packet
  int j=1;              // counter of the respond packet
  int len=incomingPacket[i++];
  //char respond[50];
  //debug("len: ");
  //debugln(len);
  while(1){
    if(i>=len){
      break;
    }
    byte b=(byte)incomingPacket[i++];
    //debugln(b);
    if(b==TEMP_REQUEST){
      readHumAndTemp();
      char tempbytes[2];
      respond[j++]=TEMP_FLAG;
      if(temp2bytes(tempbytes)){
        respond[j++]=tempbytes[0];
        respond[j++]=tempbytes[1];
      }
      else{
        respond[j++]=TEMP_SENSOR_ERROR;
        respond[j++]=TEMP_SENSOR_ERROR;
      }
    }
    if(b==HUM_REQUEST){
      readHumAndTemp();
      respond[j++]=HUM_FLAG;
      if(humidity!=-10){
        respond[j++]=humidity;
      }
      else{
        respond[j++]=HUM_SENSOR_ERR;
      }
    }
    if(b==HEATER_ON){
      //heaterCounter=incomingPacket[i++];
      turnON(((int)incomingPacket[i++])-1);
      if(heaterCounter<=1){
        heaterCounter=0;
        digitalWrite(HEATER,0);
      }

    }
    if(b==HEATER_REQUEST){
      respond[j++]=HEATER_REMAINING;
      respond[j++]=(byte)(heaterCounter/(60000/heaterHandlerPeriod)+1);
    }
    if(b==ADD_TSCH){
      TSCH_array[TSCH_len][0]=(int)(incomingPacket[i++])-1;
      TSCH_array[TSCH_len][1]=(int)(incomingPacket[i++])-1;
      TSCH_array[TSCH_len][2]=(int)(incomingPacket[i++])-1;
      EEPROM.write(TSCH_len*3+1, TSCH_array[TSCH_len][0]);
      EEPROM.write(TSCH_len*3+2, TSCH_array[TSCH_len][1]);
      EEPROM.write(TSCH_len*3+3, TSCH_array[TSCH_len][2]);
      TSCH_len++;
      EEPROM.write(0, TSCH_len);
      EEPROM.commit();
      respond[j++]=ADD_TSCH_CONF;
    }
    if(b==REMOVE_TSCH){
      int index=(int)incomingPacket[i++]-1;
      if(index>=0 && index<TSCH_len){
        respond[j++]=REMOVE_TSCH_CONF;
        for (int k=index;k<TSCH_len-1;++k){
          TSCH_array[k][0]=TSCH_array[k+1][0];
          TSCH_array[k][1]=TSCH_array[k+1][1];
          TSCH_array[k][2]=TSCH_array[k+1][2];
          EEPROM.write(k*3+1,TSCH_array[k][0]);
          EEPROM.write(k*3+2,TSCH_array[k][1]);
          EEPROM.write(k*3+3,TSCH_array[k][2]);
        }
        TSCH_len--;
        EEPROM.write(0, TSCH_len);
        EEPROM.commit();
      }
      else{
        respond[j++]=REMOVE_TSCH_NCONF;
      }
    }
    if(b==GET_TSCHS){
      respond[j++]=SEND_TSCHS;
      respond[j++]=TSCH_len+1;
      for (int k=0;k<TSCH_len;++k){
          respond[j++]=TSCH_array[k][0]+1;
          respond[j++]=TSCH_array[k][1]+1;
          respond[j++]=TSCH_array[k][2]+1;
       }
    }
    
  }
  if(j==1){
    respond[j++]=MSG_RECEIVED;
  }
  respond[j]='\0';
  respond[0]=j-1;
  delay(1);                         // without this delay the controller doesn t send back the msg properly
  incomingPacket[0]=0;
  //return respond;
}

void getTime(){

  time_t now = time(nullptr);         // get the current time
  struct tm* p_tm = localtime(&now);  // convert to local time


  currentTime=p_tm->tm_hour*60+p_tm->tm_min;


}

int receiveUDP(){
  int packetSize = Udp.parsePacket();
  if (packetSize)
  {
    // receive incoming UDP packets
    //Serial.printf("Received %d bytes from %s, port %d\n", packetSize, Udp.remoteIP().toString().c_str(), Udp.remotePort());
    //const char* cRemIP=Udp.remoteIP().toString().c_str();
    //remoteIp=Udp.remoteIP();
    //const char* cRemIP=Udp.remoteIP();
    //debugln(typeid(Udp.remoteIP()).name());
    //strcpy(remoteIp,cRemIP);
    int len = Udp.read(incomingPacket, 255);
    debugln(len);
    debugln((int)incomingPacket[1]);
    if((int)incomingPacket[1]!=BOSS){
        return 0;
    }
    if (len > 0)
    {
      incomingPacket[len] = 0;
    }
    //Serial.printf("UDP packet contents: %s\n", incomingPacket);
  }
  return packetSize;
}



void sendUDP(){
    Udp.beginPacket(Udp.remoteIP(), localUdpPort);
    Udp.write(respond);
    Udp.endPacket();  


    debugln("");
    debug("Respond:  ");
    for(int i=0;i<strlen(respond);++i){
      debug((byte)respond[i]);
      debug(" ");
    }
    debugln("");
/*
    Serial.print(Udp.remoteIP());
    Serial.print(" ");
    Serial.println(Udp.remotePort());
  */  
}


/*
void sendUDP(char *msg,char* ip){  
    Udp.beginPacket(ip, localUdpPort);
    Udp.write(msg);
    Udp.endPacket();
}
*/

void readHumAndTemp(){
  //long startTime=millis();
  humidity = dht.readHumidity();
  float temp= dht.readTemperature();
  //temperature = dht.computeHeatIndex(temp, humidity, false);
  temperature=temp;
  //startTime=millis()-startTime;
  //debug("measure time: ");
  //debugln(startTime);
  if(isnan(humidity)){
    humidity=-10;
  }
  if(isnan(temperature)){
    temperature=-10;
  }
}

bool temp2bytes(char*tempBytes){
  if(temperature!=-10){
    double fractpart, intpart; 
    fractpart = modf (temperature , &intpart);
    tempBytes[0]=(char)intpart;
    tempBytes[1]=(char)(fractpart*100);
    return true;
  }
  return false;
}

void turnON(int minutes){
    digitalWrite(HEATER,1);
    heaterCounter=minutes*60000/heaterHandlerPeriod;
}



void loop() {
  ArduinoOTA.handle();

  /*
    if(receiveUDP()>0){
    debug("received: ");
    //char*respond=decodeAndBuildRespond();
    decodeAndBuildRespond();
    delay(100);
    //sendUDP(remoteIp);
    sendUDP();
  }
  */

    

  if(millis()-universalTimer>1000){
    universalTimer=millis();

    if(millis()-reconnectTimer>reconnect_period){
      reconnectTimer=millis();
      connect2Wifi(200);
    }
    if(millis()-date_check_timer>60000){
      getTime();
      checkTSCH();
      if(strcmp(timeStamp.c_str(),"04:00")==0 || strcmp(timeStamp.c_str(),"16:00")==0){
        debugln("restarting");
        ESP.restart();
      }
      date_check_timer=millis();
    }
    if(millis()-heaterTimer>heaterHandlerPeriod){
      heaterHandler();
      heaterTimer=millis();
    }
    if(millis()-tempSampleTimer>tempSamplePeriod){
        //if(startMeasuring==true){
            tempSampleTimer=millis();
            readHumAndTemp();
            startMeasuring=false;
          //}
      }
    notifyClients(heaterCounter, temperature);
   }
   
  
}


