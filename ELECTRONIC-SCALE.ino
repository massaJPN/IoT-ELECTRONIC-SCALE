#include <M5GFX.h>
#include <M5StickC.h>
#include "HX711.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h> //version 6.19.4

#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID "******"//blynkのTemplate ID
#define BLYNK_DEVICE_NAME "******"//blynkのDevice name
char auth[] = "*****2";//blynkのAuth Taken
#include <BlynkSimpleEsp32.h>

char *ssid = "********";// Wi-FiのSSID
char *password = "********";// Wi-Fiのパスワード

const char* published_url = "https://script.google.com/macros/s/**********/exec";// GoogleスプレッドシートのデプロイされたURLを設定

M5GFX display;
M5Canvas canvas(&display);

// HX711 接続先PIN.
#define LOADCELL_DOUT_PIN 33
#define LOADCELL_SCK_PIN  32

HX711 scale;

void setup_wifi(){
  
  Serial.println("Connecting to ");
  Serial.print(ssid);

  WiFi.disconnect();
  delay(500);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED){
    delay(500);
  }
  Serial.println("\nWiFi Connected.");
  canvas.setTextSize(2);
  canvas.setFont(&fonts::Font0); 
  canvas.setCursor(5, 56); 
  canvas.println("WiFi Connected.");
  canvas.println("IP address: ");  
  canvas.println(WiFi.localIP());
  canvas.pushSprite(0, 0);
  delay(2000);
}

void setup() {
    M5.begin();
    display.begin();
    display.setRotation(3);
    canvas.setColorDepth(1);
    canvas.createSprite(display.width(), display.height());
    canvas.setTextDatum(MC_DATUM);
    canvas.setPaletteColor(1, GREEN);
    canvas.setTextSize(2);
    canvas.setFont(&fonts::Font0); 
    canvas.setCursor(5, 40); 
    canvas.print("Calibration sensor.");
    canvas.pushSprite(0, 0);
    scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
    scale.set_scale(27.61f);  // set scale
    scale.tare();             // auto set offset
    
    setup_wifi();

    canvas.setTextSize(2);
    canvas.println("Blynk connecting..");
    canvas.pushSprite(0, 0);
    delay(1000);
    Blynk.begin(auth, ssid, password);
    canvas.println("Blynk connected.");
    canvas.pushSprite(0, 0);
    delay(2000);
}

BLYNK_WRITE(V1)// Blynk で設定した端子名
{
  float weight = scale.get_units(10) / 1000.0;
  int value = param.asInt();
  if (value == 1 ){
 　　　Serial.printf("blunk-read = %d", value);
    canvas.fillSprite(BLACK);
    canvas.setTextSize(2);
    canvas.setFont(&fonts::Font0); 
    canvas.setCursor(10, 20); 
    canvas.print("Sending...");
    canvas.pushSprite(0, 0);
    delay(1000);      

    StaticJsonDocument<500> doc;
    char pubMessage[256];   
	
      // JSONメッセージの作成
      JsonArray idValues = doc.createNestedArray("ID");
      idValues.add("12345"); //名前など
      JsonArray dataValues = doc.createNestedArray("weight");
      dataValues.add(weight);

      serializeJson(doc, pubMessage);

      // HTTP通信開始
      HTTPClient http;

      Serial.print(" HTTP通信開始　\n");
      http.begin(published_url);
   
      Serial.print(" HTTP通信POST　\n");
      int httpCode = http.POST(pubMessage);
   
      if(httpCode > 0){
        canvas.setTextSize(2);
        canvas.setFont(&fonts::Font0); 
        canvas.setCursor(13, 95); 
        canvas.print("HTTP Response: "+String(httpCode)); 
        if(httpCode == HTTP_CODE_OK){
          canvas.setTextSize(2);
          canvas.setFont(&fonts::Font0); 
          canvas.setCursor(13, 115); 
          canvas.print("HTTP Success!!");
          String payload = http.getString();
          Serial.println(payload);
        }
      }else{
        canvas.setTextSize(2);
        canvas.setFont(&fonts::Font0); 
        canvas.setCursor(13, 115); 
        canvas.print("FAILED!!");       
        Serial.printf(" HTTP failed,error: %s\n", http.errorToString(httpCode).c_str());
      }
    http.end();
    Blynk.virtualWrite(V1, LOW); 
  }
}

char info[100];
int x;
void loop() {
    Blynk.run();
    canvas.fillSprite(BLACK);
    canvas.setTextSize(2);
    canvas.setFont(&fonts::Font0); 
    canvas.setCursor(13, 20); 
    canvas.print("<-Upload");
    canvas.setCursor(90, 115); 
    canvas.print("Tare");
            
    float weight = scale.get_units(10) / 1000.0;
    canvas.setTextSize(1);
    canvas.setFont(&fonts::Font7);  //7セグフォント選択
    if (weight >= 0) {
        Serial.printf("%.1f", weight);
        sprintf(info, "%.1f", weight);
        int len = String(info).length();
        Serial.print(" len");
        Serial.println(len);
        if (len == 3){
            x = 75;
          }
          else if (len == 4){
            x = 43;
          }
          else if (len == 5){
            x = 11;
          }
        canvas.setCursor(x, 65);
        canvas.print(String(info));
        canvas.setTextSize(4);
        canvas.setFont(&fonts::Font0);
        canvas.setCursor(170, 75);
        canvas.print("kg");
    } else {
        canvas.setCursor(75, 65);
        canvas.print("0.0");
        canvas.setFont(&fonts::Font0); 
        canvas.setTextSize(4);
        canvas.setCursor(170, 75);
        canvas.print("kg");
    }
    M5.update();
    
    if (M5.BtnB.wasPressed()) {
      canvas.setFont(&fonts::Font0); 
      canvas.setCursor(50, 115); 
      canvas.setTextSize(2);
      canvas.print("Zero Cal");
      canvas.pushSprite(0, 0);
      scale.tare();
      canvas.setTextSize(2);
      canvas.setCursor(50+108, 115); 
      canvas.print("done");
      delay(1000);
    }

    StaticJsonDocument<500> doc;
    char pubMessage[256];
    if (M5.BtnA.wasPressed()) {
      canvas.fillSprite(BLACK);
      canvas.setTextSize(2);
      canvas.setFont(&fonts::Font0); 
      canvas.setCursor(10, 20); 
      canvas.print("Sending...");
      canvas.pushSprite(0, 0);
      delay(1000);      
       
      // JSONメッセージの作成
      JsonArray idValues = doc.createNestedArray("ID");
      idValues.add("12345"); //名前など

      JsonArray dataValues = doc.createNestedArray("weight");
      dataValues.add(weight);

      serializeJson(doc, pubMessage);

      // HTTP通信開始
      HTTPClient http;

      Serial.print(" HTTP通信開始　\n");
      http.begin(published_url);
   
      Serial.print(" HTTP通信POST　\n");
      int httpCode = http.POST(pubMessage);
   
      if(httpCode > 0){
        canvas.setTextSize(2);
        canvas.setFont(&fonts::Font0); 
        canvas.setCursor(13, 95); 
        canvas.print("HTTP Response: "+String(httpCode));
        if(httpCode == HTTP_CODE_OK){
          canvas.setTextSize(2);
          canvas.setFont(&fonts::Font0); 
          canvas.setCursor(13, 115); 
          canvas.print("HTTP Success!!");
          String payload = http.getString();
          Serial.println(payload);
        }
      }else{
        canvas.setTextSize(2);
        canvas.setFont(&fonts::Font0); 
        canvas.setCursor(13, 115); 
        canvas.print("FAILED!!");       
        Serial.printf(" HTTP failed,error: %s\n", http.errorToString(httpCode).c_str());
      }
    http.end();
    }
    canvas.pushSprite(0, 0);
    delay(1000);  
}
