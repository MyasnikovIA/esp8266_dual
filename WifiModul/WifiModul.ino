typedef void (*Callback)();
void setup() {
    Serial.begin(115200);
    wifiSetup("esp8266","123456789");
}


int ind = 0;
void loopConnect() {
    String line = readWiFiData();
    Serial.println(line);
    ind+=1;
    String msg= "{\"OK\":\"" + String(ind) + "\"}";
    sendClient(msg);
}


void loop() {
    wifiLoop();
    connectServer(loopConnect);
}
