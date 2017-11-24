// Specifically for use with the Adafruit Feather, the pins are pre-set here!

// include SPI, MP3 and SD libraries
#include <SPI.h>
//#include <SD.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#include <Adafruit_VS1053_mod.h>

#include <SdFat.h>
//#include <SdFatUtil.h>

//SdFat SD_CARD;

#define DBG_OUTPUT_PORT Serial
//#define SD_CARD SD
#define STATUS 2

#define ANA_PIN A0
#define PLAY_PIN 5
#define VOL_PIN 13

#define VOL_DOWN_VALUE 1
#define SKIP_VALUE 2
#define VOL_UP_VALUE 3




bool IS_PLAYING = false;
String global_track;
String global_modi;
int global_volume;

const char* ssid = "add SSID";
const char* password = "add WPA2 Key";
const char* host = "federesp";

ESP8266WebServer server(80);

static bool hasSD = false;
File uploadFile;


// These are the pins used
#define VS1053_RESET   -1     // VS1053 reset pin (not used!)

// Feather M0 or 32u4
#if defined(__AVR__) || defined(ARDUINO_SAMD_FEATHER_M0)
  #define VS1053_CS       6     // VS1053 chip select pin (output)
  #define VS1053_DCS     10     // VS1053 Data/command select pin (output)
  #define CARDCS          5     // Card chip select pin
  // DREQ should be an Int pin *if possible* (not possible on 32u4)
  #define VS1053_DREQ     9     // VS1053 Data request, ideally an Interrupt pin

// Feather ESP8266
#elif defined(ESP8266)
  #define VS1053_CS      16     // VS1053 chip select pin (output)
  #define VS1053_DCS     15     // VS1053 Data/command select pin (output)
  #define CARDCS          2     // Card chip select pin
  #define VS1053_DREQ     0     // VS1053 Data request, ideally an Interrupt pin

// Feather ESP32
#elif defined(ESP32)
  #define VS1053_CS      32     // VS1053 chip select pin (output)
  #define VS1053_DCS     33     // VS1053 Data/command select pin (output)
  #define CARDCS         14     // Card chip select pin
  #define VS1053_DREQ    15     // VS1053 Data request, ideally an Interrupt pin

// Feather Teensy3
#elif defined(TEENSYDUINO)
  #define VS1053_CS       3     // VS1053 chip select pin (output)
  #define VS1053_DCS     10     // VS1053 Data/command select pin (output)
  #define CARDCS          8     // Card chip select pin
  #define VS1053_DREQ     4     // VS1053 Data request, ideally an Interrupt pin

// WICED feather
#elif defined(ARDUINO_STM32_FEATHER)
  #define VS1053_CS       PC7     // VS1053 chip select pin (output)
  #define VS1053_DCS      PB4     // VS1053 Data/command select pin (output)
  #define CARDCS          PC5     // Card chip select pin
  #define VS1053_DREQ     PA15    // VS1053 Data request, ideally an Interrupt pin

#elif defined(ARDUINO_FEATHER52)
  #define VS1053_CS       30     // VS1053 chip select pin (output)
  #define VS1053_DCS      11     // VS1053 Data/command select pin (output)
  #define CARDCS          27     // Card chip select pin
  #define VS1053_DREQ     31     // VS1053 Data request, ideally an Interrupt pin
#endif


Adafruit_VS1053_FilePlayer musicPlayer = 
  Adafruit_VS1053_FilePlayer(VS1053_RESET, VS1053_CS, VS1053_DCS, VS1053_DREQ, CARDCS);


void getDirectory(String path, String now_track = "");

void returnOK() {
  server.send(200, "text/json", "");
}

void returnOKjson() {
  server.send(200, "text/json", "");
}

void returnFail(String msg) {
  server.send(500, "text/plain", msg + "\r\n");
}


String createJSON(String *var,String *val, int len, String append_json) {

  String output = "";
  if (append_json == "") {
     output = "";
  } else {
   
    output = append_json+",";
  }
  
  int i=0;
  output += "{";
  for(i=0;i<len;i++) { 
        output += "\"";
        output += var[i];
        output += "\":\"";
        output += val[i];
        output += "\"";
        if (i < (len-1))
            output += ",";
    }
   
    output += "}";
    return output;
}

void sendJSON(String JSON) {

  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/json", "");

  server.sendContent("[");
  server.sendContent(JSON);
  server.sendContent("]");
  
}

void processVolumeRequest(){
  if(!server.hasArg("v")) return returnFail("BAD ARGS");
  int volume = server.arg("v").toInt();

  int ret_vol = changeVolume(volume);

  String charVolume = String(ret_vol);
  String JSON;
  int len = 1;
  String var[1] = { "volume" };
  String val[1] = { charVolume };

  JSON = createJSON(var,val,len,"");
  sendJSON(JSON);

}

int changeVolume(int vol_val) {

  if ((vol_val < 0) && (global_volume == 0)) {
    return -1;
  } else if ((vol_val > 0) && (global_volume == 100)) {
    return -1;
  } else {
    global_volume = global_volume + vol_val;
    musicPlayer.setVolume(global_volume,global_volume);
    DBG_OUTPUT_PORT.println("Changed Volume to:");
    DBG_OUTPUT_PORT.println(String(global_volume));
    return global_volume;
  }
}

void setGlobal(String track, String modi) {
  global_track = track;
  global_modi = modi;
}

int playSong(String modi, String track) {

    if (modi=="start") {
      CheckAndRestart();
      musicPlayer.startPlayingFile(track.c_str());
      DBG_OUTPUT_PORT.println("Start track:");
      DBG_OUTPUT_PORT.println(track);
    }
  
    else if (modi=="next") {
      CheckAndRestart();     
      String next_track = GetDirectory("/",track);
      if (next_track.endsWith("MP3")) {
        musicPlayer.startPlayingFile(next_track.c_str());
        DBG_OUTPUT_PORT.println("Next Track:");
        DBG_OUTPUT_PORT.println(next_track);
      }
      track = next_track;
    }
    
    else if (modi=="pause") {
      musicPlayer.pausePlaying(true);
      DBG_OUTPUT_PORT.println("PAUSED");
    }

    else if (modi=="resume") {
      musicPlayer.pausePlaying(false);
      DBG_OUTPUT_PORT.println("RESUMED");
    }
    
    else
      return -1;
  
    setGlobal(track,modi);
    return 1;
    
}


void processSongRequest(){
  
  if(!server.hasArg("m")) return returnFail("BAD ARGS");
  String modi = server.arg("m");
  String track = server.arg("t");
  int result = 0;
  
  result = playSong(modi,track);

    int len = 2;
    String var[2] = { "modi","track" };
    String val[2] = { global_modi,global_track.c_str() };


  String JSON = createJSON(var,val,len,"");
  DBG_OUTPUT_PORT.println(JSON);
  
  sendJSON(JSON);

}

void CheckAndRestart() {
    
  if (musicPlayer.playingMusic) {
    DBG_OUTPUT_PORT.println("musicplayer is playing");
    musicPlayer.stopPlaying();
    DBG_OUTPUT_PORT.println("musicplayer stopped gracefully");
    delay(10);
    musicPlayer.begin();

    }
}

void processStatusRequest(){

  String *var;
  String *val;
  int len;
  
  if (musicPlayer.playingMusic) {
      int len = 2;
      String var[2] = { "modi","track" };
      String val[2] = { global_modi,global_track.c_str() };
  } else {
      int len = 1;
      String var[1] = { "modi"};
      String val[1] = { "IDLE" };
  }
  String JSON;
  JSON = createJSON(var,val,len,"");
  DBG_OUTPUT_PORT.println("Inside Status JSON:");
  DBG_OUTPUT_PORT.println(JSON);
  sendJSON(JSON);
 
}

void processFilesRequest() {
  
  if(!server.hasArg("dir")) return returnFail("BAD ARGS");
  String path = server.arg("dir");
  if(path != "/" && !musicPlayer.SD_CARD.exists((char *)path.c_str())) return returnFail("BAD PATH");

  File dir = musicPlayer.SD_CARD.open((char *)path.c_str());
  path = String();
  if(!dir.isDirectory()){
    dir.close();
    return returnFail("NOT DIR");
  }
  
  WiFiClient client = server.client();


  String JSON = GetDirectory(path,"");
  sendJSON(JSON);

}
  
String GetDirectory(String path, String track){
  
  SdFile dir;
  
  path = String();

  SdFile entry;

  String track_json = "";
  String temp_json = "";
  String first_track = "";
  boolean track_found = false;
  uint16_t n = 0;
  char temp_name[50];


  if (!dir.open("/",O_READ)) {
    musicPlayer.SD_CARD.errorHalt("open root failed");
  }
  
  while (1) {
    entry.openNext(&dir,O_READ);
    if (track.length() == 0 && !entry.isFile()) break;
    else if (track.length() > 0 && !entry.isFile()){
        DBG_OUTPUT_PORT.println("return first track");
        DBG_OUTPUT_PORT.println(track.length());
        return first_track;
    }

    entry.getName(temp_name,50);

    String entry_type = (entry.isDir()) ? "dir" : "file";
    String str_entry(temp_name);
    int len = 2;
    String var[2] = {"type","name"};
    String val[2] = {entry_type,str_entry};

    //DBG_OUTPUT_PORT.println(str_entry);

    if (n == 0) {
        track_json = createJSON(var,val,len,"");
        temp_json = track_json;

    } else {

      track_json = createJSON(var,val,len,temp_json);
      temp_json = track_json;

    }

   
    
    if (track_found && str_entry.endsWith("MP3")) {
      DBG_OUTPUT_PORT.println("test return");
      return str_entry;
    }

    if (str_entry.endsWith("MP3") || str_entry.endsWith("mp3")) {
      if (first_track == "") { 
        first_track = str_entry;
      }
 
       track.toLowerCase();
       str_entry.toLowerCase();
      if (track.equals(str_entry)) {
        DBG_OUTPUT_PORT.println("TRACK_FOUND");
        track_found = true;
      }
    }
    n++;
    entry.close();
 
 }
 DBG_OUTPUT_PORT.println("return track_json");
 return track_json;
}

bool loadFromSdCard(String path){
  String dataType = "text/plain";
  DBG_OUTPUT_PORT.println("loadFromSdCard");
  if(path.endsWith("/")) path += "index.htm";

  if(path.endsWith(".src")) path = path.substring(0, path.lastIndexOf("."));
  else if(path.endsWith(".htm")) dataType = "text/html";
  else if(path.endsWith(".css")) dataType = "text/css";
  else if(path.endsWith(".js")) dataType = "application/javascript";
  else if(path.endsWith(".png")) dataType = "image/png";
  else if(path.endsWith(".gif")) dataType = "image/gif";
  else if(path.endsWith(".jpg")) dataType = "image/jpeg";
  else if(path.endsWith(".ico")) dataType = "image/x-icon";
  else if(path.endsWith(".xml")) dataType = "text/xml";
  else if(path.endsWith(".pdf")) dataType = "application/pdf";
  else if(path.endsWith(".zip")) dataType = "application/zip";

  File dataFile = musicPlayer.SD_CARD.open(path.c_str());
  if(dataFile.isDirectory()){
    path += "/index.htm";
    dataType = "text/html";
    dataFile = musicPlayer.SD_CARD.open(path.c_str());
  }

  if (!dataFile)
    return false;

  if (server.hasArg("download")) dataType = "application/octet-stream";

  if (server.streamFile(dataFile, dataType) != dataFile.size()) {
    DBG_OUTPUT_PORT.println("Sent less data than expected!");
  }

  dataFile.close();
  return true;
}

void deleteRecursive(String path){
  File file = musicPlayer.SD_CARD.open((char *)path.c_str());
  if(!file.isDirectory()){
    file.close();
    musicPlayer.SD_CARD.remove((char *)path.c_str());
    return;
  }

  file.rewindDirectory();
  while(true) {
    File entry = file.openNextFile();
    if (!entry) break;
    String entryPath = path + "/" +entry.name();
    if(entry.isDirectory()){
      entry.close();
      deleteRecursive(entryPath);
    } else {
      entry.close();
      musicPlayer.SD_CARD.remove((char *)entryPath.c_str());
    }
    yield();
  }

  musicPlayer.SD_CARD.rmdir((char *)path.c_str());
  file.close();
}

void handleNotFound(){
  DBG_OUTPUT_PORT.println("handlenotfound");
  if(hasSD && loadFromSdCard(server.uri())) return;
  String message = "SDCARD Not Detected\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " NAME:"+server.argName(i) + "\n VALUE:" + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  DBG_OUTPUT_PORT.print(message);
}

void handleDelete(){
  if(server.args() == 0) return returnFail("BAD ARGS");
  String path = server.arg(0);
  if(path == "/" || !musicPlayer.SD_CARD.exists((char *)path.c_str())) {
    returnFail("BAD PATH");
    return;
  }
  deleteRecursive(path);
  returnOK();
}

void handleCreate(){
  if(server.args() == 0) return returnFail("BAD ARGS");
  String path = server.arg(0);
  if(path == "/" || musicPlayer.SD_CARD.exists((char *)path.c_str())) {
    returnFail("BAD PATH");
    return;
  }

  if(path.indexOf('.') > 0){
    File file = musicPlayer.SD_CARD.open((char *)path.c_str(), FILE_WRITE);
    if(file){
      file.write((const char *)0);
      file.close();
    }
  } else {
    musicPlayer.SD_CARD.mkdir((char *)path.c_str());
  }
  returnOK();
}

void handleFileUpload(){
  if(server.uri() != "/edit") return;
  HTTPUpload& upload = server.upload();
  
  if(upload.status == UPLOAD_FILE_START){
    if(musicPlayer.SD_CARD.exists((char *)upload.filename.c_str())) musicPlayer.SD_CARD.remove((char *)upload.filename.c_str());
    uploadFile = musicPlayer.SD_CARD.open(upload.filename.c_str(), FILE_WRITE);
    DBG_OUTPUT_PORT.print("Upload: START, filename: "); DBG_OUTPUT_PORT.println(upload.filename);
  } else if(upload.status == UPLOAD_FILE_WRITE){
    if(uploadFile) {
      uploadFile.write(upload.buf, upload.currentSize);

      int len = 1;
      String var[1] = {"size"};
      String val[1] = {(String)upload.totalSize};
      
      String output = createJSON(var,val,len,"");
      
      server.sendContent(output);
      server.sendContent("]");
  
    }
    DBG_OUTPUT_PORT.print("Upload: WRITE, Bytes: "); DBG_OUTPUT_PORT.println(upload.currentSize);
  } else if(upload.status == UPLOAD_FILE_END){
    if(uploadFile) uploadFile.close();
    DBG_OUTPUT_PORT.print("Upload: END, Size: "); DBG_OUTPUT_PORT.println(upload.totalSize);
  }
 
  
}


int initMusicHat() {
  if (! musicPlayer.begin()) { // initialise the music player
    DBG_OUTPUT_PORT.println(F("Couldn't find VS1053, do you have the right pins defined?"));
    //DBG_SERVER_LOG.
    return -1;
  }
  musicPlayer.sineTest(0x44, 500);    // Make a tone to indicate VS1053 is working

/*
  if (!musicPlayer.SD_CARD.begin(CARDCS)) {
    DBG_OUTPUT_PORT.println(F("SD failed, or not present"));
    //DBG_SERVER_LOG.
    return -1;
  }
  */
  hasSD = true;

  // Set volume for left, right channels. lower numbers == louder volume!
  global_volume = 40;
  
  musicPlayer.setVolume(global_volume,global_volume);

  #if defined(__AVR_ATmega32U4__) 
  // Timer interrupts are not suggested, better to use DREQ interrupt!
  // but we don't have them on the 32u4 feather...
  musicPlayer.useInterrupt(VS1053_FILEPLAYER_TIMER0_INT); // timer int
  #elif defined(ESP32)
  // no IRQ! doesn't work yet :/
  #else
  // If DREQ is on an interrupt pin we can do background
  // audio playing
  musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT);  // DREQ int
  #endif

}

int initWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  DBG_OUTPUT_PORT.print("Connecting to ");
  DBG_OUTPUT_PORT.println(ssid);

  // Wait for connection
  uint8_t i = 0;
  while (WiFi.status() != WL_CONNECTED && i++ < 20) {//wait 10 seconds
    delay(500);
  }
  if(i == 21){
    DBG_OUTPUT_PORT.print("Could not connect to");
    DBG_OUTPUT_PORT.println(ssid);
    while(1) delay(500);
  }
  DBG_OUTPUT_PORT.print("Connected! IP address: ");
  DBG_OUTPUT_PORT.println(WiFi.localIP());

  return 1;
  
}

int initServer() {


  if (MDNS.begin(host)) {
    MDNS.addService("http", "tcp", 80);
    DBG_OUTPUT_PORT.println("MDNS responder started");
    DBG_OUTPUT_PORT.print("You can now connect to http://");
    DBG_OUTPUT_PORT.print(host);
    DBG_OUTPUT_PORT.println(".local");
  }

  server.on("/volume",HTTP_GET, processVolumeRequest);
  server.on("/status",HTTP_GET, processStatusRequest);
  server.on("/music", HTTP_GET, processSongRequest);
  server.on("/list",  HTTP_GET, processFilesRequest);
  server.on("/edit",  HTTP_DELETE, handleDelete);
  server.on("/edit",  HTTP_PUT, handleCreate);
  server.on("/edit",  HTTP_POST, [](){ returnOKjson(); }, handleFileUpload);
  server.onNotFound(handleNotFound);

  server.begin();
  DBG_OUTPUT_PORT.println("HTTP server started");

  
}

int initButtons() {
 
  pinMode(ANA_PIN,INPUT);
  pinMode(PLAY_PIN,INPUT);
 // pinMode(VOL_PIN,INPUT);
 
}

void error() {

  while(1) {
    digitalWrite(STATUS, HIGH);
    delay(1000);
    digitalWrite(STATUS, LOW);
    delay(1000);
  }
}

void initAck() {

   digitalWrite(STATUS, HIGH);
   delay(500);
   digitalWrite(STATUS, LOW);
   delay(500);
}

void setup() {
  DBG_OUTPUT_PORT.begin(115200);
  DBG_OUTPUT_PORT.println("\n\nAudioPet V0.1");
  pinMode(STATUS, OUTPUT);

  
  if (! initWifi()) {
    error();
  } else {
    initAck();
  }

  if (! initServer()) {
    error();
  } else {
    initAck();
  }

    if (! initMusicHat()) {
    error();
  } else {
    initAck();
  }



   initButtons();
}

int processAnalogButton(int ana_val) {

  if ((ana_val > 600) && (ana_val < 700)) {
    return VOL_UP_VALUE;
  }

  else if ((ana_val > 700) && (ana_val < 950)) {
    return SKIP_VALUE;
  }

  else if ((ana_val > 950) && (ana_val < 1100)) {
    return VOL_DOWN_VALUE; 
  }
}

void doAnalogAction(int button_choice) {

  if (button_choice == VOL_UP_VALUE) {
    changeVolume(-10);
  }

  if (button_choice == SKIP_VALUE) {
    playSong("next",global_track);
  }

  if (button_choice == VOL_DOWN_VALUE) {
    changeVolume(10);
  }
}

void SkipInTrack(uint8_t offset) {
  uint16_t playtime = 0;
  uint16_t after_skip = 0;
  playtime= musicPlayer.decodeTime();
  DBG_OUTPUT_PORT.println("Playtime before Skip:");
  DBG_OUTPUT_PORT.println(playtime);
  after_skip = musicPlayer.skip(playtime+offset);
  DBG_OUTPUT_PORT.println("Playtime after Skip:");
  DBG_OUTPUT_PORT.println(after_skip);
}

void loop() {
  
  server.handleClient();

  int ButtonRead = analogRead(ANA_PIN);
  int already_pressed;
  if (ButtonRead > 200) {
    DBG_OUTPUT_PORT.print("ANA_PIN:");
    DBG_OUTPUT_PORT.print(ButtonRead);
    int button_choice = processAnalogButton(ButtonRead);
    delay(200);
    ButtonRead = analogRead(ANA_PIN);
    if ((button_choice == processAnalogButton(ButtonRead)) && button_choice == SKIP_VALUE) {
       DBG_OUTPUT_PORT.println("Skip in Track");
       already_pressed = 1;
       SkipInTrack(10);
    }
    else if (already_pressed == 0) {
      already_pressed = 0;
      doAnalogAction(button_choice);
    }
  }

    if (digitalRead(PLAY_PIN)) {
      DBG_OUTPUT_PORT.println("PLAY_PIN:");
      delay(150);
      if (musicPlayer.playingMusic) {
        playSong("pause","");
      } else if ( global_modi == "pause") {
        playSong("resume",""); 
      } else {
        String track = GetDirectory("/","F");
        DBG_OUTPUT_PORT.println(track);
        playSong("start",track);
      }
    }

 

  /*if (musicPlayer.stopped()) {
    int x = 1;
  }*/



}


