/*

void setup() {
    wifiSetup("esp8266","123456789");
}

void loop() {
    wifiLoop(); // если нет подключения к точке доступа, тогда  запускается внутренняя точка доступа и безконечный цикл,  остальное исполнение кода не произойдет
    ---- код обработки --
}


*/

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>
#include <EasyButton.h>
#include <ArduinoJson.h> // https://arduinojson.org/v6/api/jsonobject/containskey/
#define BUTTON_PIN 0            //  кнопка Flash
EasyButton button(BUTTON_PIN);  //  кнопка Flash
MDNSResponder mdns;
ESP8266WebServer serverWifiManager(80);
WiFiClient clientSocket;
DynamicJsonDocument jsoonObj(2048);
String Essid = "";                 //EEPROM Network SSID
String Epass = "";                 //EEPROM Network Password
String sssid = "";                 //Read SSID From Web Page
String passs = "";                 //Read Password From Web Page
String localIPStr = "";
bool isSetupMode = false;          // режим настройки устройства
bool DEBUG = true;
typedef void (*Callback) ();
Callback responseLoop;

void ClearEeprom(){
    // Serial.println("Clearing Eeprom");
    for (int i = 0; i < 2048; ++i) { EEPROM.write(i, 0); }
    delay(100);
    EEPROM.commit();
}

String webPage = "";
void wifiSetup(String ssidApp,String passApp){
    EEPROM.begin(2048);
    // ClearEeprom();
    button.begin();   
    button.onPressed(onPressedFlash);//  кнопка сброса "Flash"
    delay(2000);
    String line=""; 
    for (int i = 0; i < 2048; ++i) {
        line += char(EEPROM.read(i)); 
    }
    DeserializationError error = deserializeJson(jsoonObj, line);
    //  servoPin = String(jsoonObj["ssid"]).toInt();
    //  if (jsoonObj.containsKey("from")) fromDevice = String(jsoonObj["from"]);
    String Essid = String(jsoonObj["ssid"]);
    String Epass = String(jsoonObj["pass"]);
    if ((Essid!="")&&(Essid!="null")) {  
        int ssid_len = Essid.length() + 1; 
        char ssid_array[ssid_len];
        Essid.toCharArray(ssid_array, ssid_len);
        int pass_len = Epass.length() + 1; 
        char pass_array[pass_len];
        Epass.toCharArray(pass_array, pass_len);
        WiFi.begin(Essid.c_str(), Epass.c_str());   //c_str()
        // WiFi.begin(ssid_array, pass_array); 
        // WiFi.begin("ELTEX-87A2_MOB","GP08004568");
        int countTest = 0;
        while (WiFi.status() != WL_CONNECTED) {
            countTest++;
            Serial.print(".");
            delay(500);
        }
        if (DEBUG) {
            Serial.println("");
            Serial.println(F("[CONNECTED]"));
            Serial.print("[IP ");              
            Serial.print(WiFi.localIP()); 
            Serial.println("]");
        }
        IPAddress ip = WiFi.localIP();           //Get ESP8266 IP Adress
        localIPStr = WiFi.localIP()+"";
        isSetupMode = false;
        // ----- test reconfig setup ----------
        serverWifiManager.on("/", index_html);
        serverWifiManager.on("/a",Get_Req); 
        serverWifiManager.on("/reset", get_reset); 
        // serverWifiManager.onNotFound(handle_NotFound);
        serverWifiManager.begin();
        // -----------------------------------
    } else {
        isSetupMode = true;
        IPAddress local_ip(192,168,1,1);
        IPAddress gateway(192,168,1,1);
        IPAddress subnet(255,255,255,0);
        const char* ssid = "SetupESP8266";       // SSID APP
        const char* password = "12345678";  // пароль APP
        WiFi.softAP(ssid, password);
        WiFi.softAPConfig(local_ip, gateway, subnet);
        serverWifiManager.on("/", index_html);
        serverWifiManager.on("/a",Get_Req); 
        serverWifiManager.on("/reset", get_reset); 
        // serverWifiManager.onNotFound(handle_NotFound);
        serverWifiManager.begin();
    }
}


void onPressedFlash() { //Очистка памяти EEPROM
    ClearEeprom(); //First Clear Eeprom
    EEPROM.commit();
    delay(2000);
    ESP.restart();
}

void index_html() {
    String st = "<form method='get' action='a'>";
    st += "<table>";
    st += "<tr>";
    st += "<td>";
    st += "<label>SSID: </label>";
    st += "</td>";
    st += "<td>";
    st += "<select name='ssid' >";
    int Tnetwork=0,i=0,len=0;
    Tnetwork = WiFi.scanNetworks();       //Scan for total networks available
    for (int i = 0; i < Tnetwork; ++i) {
        st += "<option value='";
        st += WiFi.SSID(i);
        st += "'>";
        st +=i + 1;
        st += ") ";
        st += WiFi.SSID(i);
        st += " (";
        st += WiFi.RSSI(i);
        st += ")";
        st += (WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ":"*";
        st += "</option>";
    }
    st += "</select>";
    st += "</td></tr>";
    st += "<tr><td><label>Password: </label></td><td><input name='pass' length='64'/></td></tr>";
    st += "<tr><td><label>Signal server IP: </label></td><td><input type='text' value='91.105.155.132' name='server_ip' pattern='^((\\d{1,2}|1\\d\\d|2[0-4]\\d|25[0-5])\\.){3}(\\d{1,2}|1\\d\\d|2[0-4]\\d|25[0-5])$'/></td></tr>";
    st += "<tr><td><label>Signal server port: </label></td><td><input type='number' value='8080' name='server_port' length='4'/></td></tr>";
    st += "<tr><td><label>Signal server path: </label></td><td><input type='text' value='Example/Terminal/esp8266_web.java' name='server_path'/></td></tr>";
    st += "<tr><td><label>Device name: </label></td><td><input type='text' length='32' value='rob'  name='device_name'/></td></tr>";
    st += "<tr><td><label>Device pass: </label></td><td><input type='text' length='32' value=''  name='device_pass'/></td></tr>";
    st += "</table>";
    st += "<input type='submit' value='Save'/>";
    st += "</form>";
    st += "<br/><a href='/reset'>reset</a>";
    String line=""; 
    for (int i = 0; i < 2048; ++i) {
        line += char(EEPROM.read(i)); 
    }
    st += "<pre>";
    st += line;
    st += "</pre>";
    serverWifiManager.send( 200 , "text/html", st);    
}
void Get_Req(){
    String server_ip="";
    String server_port="";
    String device_name="";
    String device_pass="";
    String server_path="";
    if (serverWifiManager.hasArg("ssid")
       && serverWifiManager.hasArg("pass")
       && serverWifiManager.hasArg("server_ip")
       && serverWifiManager.hasArg("server_port")
       && serverWifiManager.hasArg("device_name")
       && serverWifiManager.hasArg("device_pass")
       && serverWifiManager.hasArg("server_path")
       ){  
        sssid=serverWifiManager.arg("ssid");//Get SSID
        passs=serverWifiManager.arg("pass");//Get Password
        server_ip = serverWifiManager.arg("server_ip"); 
        server_port = serverWifiManager.arg("server_port");
        device_name = serverWifiManager.arg("device_name");
        device_pass = serverWifiManager.arg("device_pass");
        server_path = serverWifiManager.arg("server_path");
    }
    if (sssid.length()>1 && passs.length()>1){
        // ClearEeprom();//First Clear Eeprom
        delay(10);
        jsoonObj["ssid"] = sssid;
        jsoonObj["pass"] = passs;
        jsoonObj["server_ip"] = server_ip;
        jsoonObj["server_port"] = server_port;
        jsoonObj["device_name"] = device_name;
        jsoonObj["device_pass"] = device_pass;
        jsoonObj["server_path"] = server_path;
        String output;
        serializeJson(jsoonObj, output);
        for (int i = 0; i < output.length(); ++i) {
            EEPROM.write(i, output[i]);
        }
        EEPROM.commit();
        String s = "\r\n\r\n<!DOCTYPE HTML>\r\n<html><h1>Metro Store</h1> ";
        s += "<p>Password Saved... Reset to boot into new wifi</html>\r\n\r\n";
        String line=""; 
        for (int i = 0; i < 2048; ++i) {
            line += char(EEPROM.read(i)); 
        }
        DeserializationError error = deserializeJson(jsoonObj, line);        
        s += "<pre>|";
        s += "\n line:"+line;
        s += "\n sssid:";
        s += String(jsoonObj["ssid"]);
        s += "\n passs:";
        s += String(jsoonObj["pass"]);
        s += "|</pre>";
        s += "<pre>line:|";
        s += line;
        s += "|</pre>";
        s += "|</pre>";
        s += "<pre>output:|";
        s += output;
        s += "|</pre>";
        s += "</html>\r\n\r\n";
        serverWifiManager.send(200,"text/html",s);
        delay(5000);
        ESP.restart();
    }
}

void get_reset(){
    serverWifiManager.send(200,"text/html","reset device");
    delay(5000);
    ESP.restart();
}    

int indStep=0;
bool onPing(){
    indStep++;
    if (indStep > 500000) {
       indStep=0;
       clientSocket.println("ping");
       clientSocket.println();
       clientSocket.println();
       return true;
   }
   return false;
}

void sendClient(String msg) {
    // clientSocket.println("{\""+methSend+"\":\""+fromDevice+"\",\"msg\":\""+msg+"\"}");
    clientSocket.print(msg);
    clientSocket.write(0);
}

String readWiFiData(){
    String line = "";
    while(clientSocket.available()){
        // String line = clientSocket.readStringUntil('\n');
        // Serial.print(line);
        char nextChar = clientSocket.read();
        line += nextChar;
    }
    return line;
}

void connectServer(Callback responseLoopStr){
    if (clientSocket.connect(String(jsoonObj["server_ip"]),int(jsoonObj["server_port"]))){
      String msg = "TERM /"+String(jsoonObj["server_path"]);
      clientSocket.print("TERM /");
      clientSocket.println(String(jsoonObj["server_path"]));
      clientSocket.print("device_name:");
      clientSocket.println(String(jsoonObj["device_name"]));
      clientSocket.print("device_pass:");
      clientSocket.println(String(jsoonObj["device_pass"]));
      clientSocket.print("ip:");
      clientSocket.println(WiFi.localIP());
      clientSocket.write(0);
      String lineTxt = readWiFiData();
      while (clientSocket.connected()) {
          serverWifiManager.handleClient();  //  используется для реконфигурации  настроек
          if (onPing()) continue;
          if (!clientSocket.connected()){ return; }
          responseLoopStr();
      }
    }  
}

void wifiLoop() {
    if (isSetupMode) {
        while (true) {
            button.read();
            serverWifiManager.handleClient();
        }
    } else {
        button.read();
    }
}
