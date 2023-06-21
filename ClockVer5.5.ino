#include <microDS3231.h>
#include <Time.h>
#include <TimeLib.h>
#include <Wire.h>
#include "FastLED.h"
#include "DHT.h"
#include <SPI.h>
#include <WiFiNINA.h>
#include <WiFiUdp.h>

#define DHT_PIN 2              // пин подключения датчика DHT 22
#define DHTTYPE DHT22         // DHT 22
#define NUM_LEDS 88          // количество светодиодов в ленте
#define COLOR_ORDER GRB       // определение порядка цветов для ленты
#define DATA_PIN 6            // пин подключения ленты
#define TIMEZONE 7            // выбор часового пояса

const char* ssid = "*****";      // имя wifi
const char* password = "******";  // пароль wifi
const char* id = "ClockRGB";
const char* pass = "Space220";
int keyIndex = 0;  
int status = WL_IDLE_STATUS;
WiFiServer server(80); 


unsigned int localPort = 2390;      // локальный порт для прослушивания UDP-пакетов
unsigned long cur_ms   = 0;
unsigned long ms2      = 10000000UL;
unsigned int err_count = 0;
uint8_t monthDays[12]={31,28,31,30,31,30,31,31,30,31,30,31};

IPAddress timeServerIP; 

const char* ntpServerName = "time.nist.gov"; 
const int NTP_PACKET_SIZE = 48; 
byte packetBuffer[ NTP_PACKET_SIZE]; 
WiFiUDP udp;

DHT dht(DHT_PIN, DHTTYPE);
MicroDS3231 rtc;

CRGB leds[NUM_LEDS]; // Задаём значение светодиодов
                     // 0,0,0,0
                     // 1,1,1,1
                     // 1 2  3 4  5 6  7 8  9 10 11 12 13 14
byte digits[14][14] = {{ 1,1, 1,1, 1,1, 0,0, 1,1, 1,1, 1,1},  // Digit 0
                       {0,0, 0,0, 1,1, 0,0, 0,0, 0,0, 1,1},   // Digit 1
                       {0,0, 1,1, 1,1, 1,1, 1,1, 1,1, 0,0},   // Digit 2
                       {0,0, 1,1, 1,1, 1,1, 0,0, 1,1, 1,1},   // Digit 3
                       {1,1, 0,0, 1,1, 1,1, 0,0, 0,0, 1,1},   // Digit 4
                       {1,1, 1,1, 0,0, 1,1, 0,0, 1,1, 1,1},   // Digit 5
                       {1,1, 1,1, 0,0, 1,1, 1,1, 1,1, 1,1},   // Digit 6
                       {0,0, 1,1, 1,1, 0,0, 0,0, 0,0, 1,1},   // Digit 7
                       {1,1, 1,1, 1,1, 1,1, 1,1, 1,1, 1,1},   // Digit 8
                       {1,1, 1,1, 1,1, 1,1, 0,0, 1,1, 1,1},   // Digit 9 
                       {1,1, 1,1, 1,1, 1,1, 0,0, 0,0, 0,0},   // ° char
                       {1,1, 1,1, 0,0, 0,0, 1,1, 1,1, 0,0},   // C char
                       {0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0},   // None char
                      {0,0, 0,0, 0,0, 1,1, 0,0, 0,0, 0,0}};  // - char

bool Dot = true;  // состояние Dot(точек)
bool TempShow = true;
bool led_state = true;  // флаг состояния ленты
int last_digit = 0;
long ledColor = CRGB::DarkOrchid; // Используемый цвет по умолчанию


// Начальная установка //
void setup() {
  
  // Serial.begin(115200);
// Инициализауия UDP соединения с NTP сервером
  // Serial.println("Starting UDP");
  udp.begin(localPort);
  // Serial.print("Local port: ");
  // Serial.println(localPort);

  // printWifiStatus();

  Wire.begin();
  LEDS.addLeds<WS2812B, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS); // Установка типа светодиодной ленты
  LEDS.setBrightness(50); // Установка начальной яркости
  TempShow = false;
  dht.begin();
  rtc.begin();
      
}

void CreateAccessPoint() {
  status = WiFi.beginAP(id, pass);
  while(status != WL_AP_LISTENING){
    WiFi.beginAP(id,pass);
  }
  // wait 10 seconds for connection:
  delay(10000);
  // server.begin();
}
// Вывод в монитор порта состояния wifi 
void printWifiStatus() {
  // вывод SSID сети к которой подключен
  // Serial.print("SSID: ");
  // Serial.println(WiFi.SSID());

  // вывод ip адреса ардуино
  IPAddress ip = WiFi.localIP();
  // Serial.print("IP Address: ");
  // Serial.println(ip);

  // вывод уровня сигнала
  long rssi = WiFi.RSSI();
  // Serial.print("signal strength (RSSI):");
  // Serial.print(rssi);
  // Serial.println(" dBm");

  // Serial.print("To see this page in action, open a browser to http://");
  // Serial.println(ip);
}


void connect_WiFi() {
  int err_count_wf = 0;
  delay(10);
  // We start by connecting to a WiFi network
  // Serial.println();
  // Serial.print("Connecting to ");
  // Serial.println(ssid);
  
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { 
          err_count_wf++;
          if( err_count_wf > 20 ){
            // Serial.println("WiFi disconnected");
            return ;
            }
   delay(500);
    // Serial.print("."); }

  // Serial.println("");
  // Serial.println("WiFi connected");
  // Serial.println("IP address: ");
  // Serial.println(WiFi.localIP());
}
}
// выводим WEB страницу
void printWEB() {
WiFiClient client = server.available();
  if (client) {                             
    // Serial.println("new client");         
    String currentLine = "";                // создание строки для хранения входящих данных от клиента
    while (client.connected()) {            // цикл, пока клиент подключен
      if (client.available()) {             // если есть байты для чтения с клиента,
        char c = client.read();             // считать байт, затем
        // Serial.write(c);                    // ввывести его на последовательном мониторе
        if (c == '\n') {                    // если байт является символом новой строки

          // если текущая строка пуста, получим два символа новой строки подряд.
          // это конец клиентского HTTP-запроса, поэтому отправлен ответ:
          if (currentLine.length() == 0) {

            // Заголовки HTTP всегда начинаются с кода ответа 
            // и тип содержимого что будет дальше, затем пустая строка:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html; charset=utf-8;");
            client.println("Connection: close");  // the connection will be closed after completion of the response
            client.println("Refresh: 5");  // refresh the page automatically every 5 sec
            client.println();
            client.println("<!DOCTYPE HTML>");
            client.println("<html>");
            
            // название HTTP страницы
            client.print("<head><title>ClockRGB</title></head>");
            // Заголовок страницы
            client.print("<h1>Управление Часами</h1>");
            // выбор яркости в виде списка
            client.print("<div><h2>Настройка Яркости:</h2></div>");
            client.print("<div><ul><li><a href=\"/H\">25%</a></li><br><li><a href=\"/L\">50%</a></li><br><li><a href=\"/F\">75%</a></li><br><li><a href=\"/R\">100%</a></li></ul></div>");
            // выбор цвета в виде таблицы
            client.print("<div><h2>Выбор цвета:</h2></div>");
            client.print("<table> <tbody>  <tr>  <td><a href=\"/Am\">Amethyst</a></td>   <td><a href=\"/Aq\">Aqua</a></td>  <td><a href=\"/RED\">Red</a></td>  <td><a href=\"/C\">Crimson</a></td>"
            "</tr>  <tr>   <td><a href=\"/DM\">DarkMagenta</a></td>   <td><a href=\"/DP\">DeepPink</a></td>    <td><a href=\"/CY\">Cyan</a></td>    <td><a href=\"/DGr\">DarkGoldenrod</a></td>   </tr>"
            "<tr> <td><a href=\"/MB\">MidnightBlue</a></td> <td><a href=\"/Green\">DarkGreen</a></td> <td><a href=\"/N\">Navy</a></td> <td><a href=\"/Ch\">Chocolate</a></td> </tr>"
            "<tr> <td><a href=\"/O\">OrangeRed</a></td> <td><a href=\"/LM\">Lime</a></td> <td><a href=\"/RB\">RoyalBlue</a></td> <td><a href=\"/T\">Tomato</a></td> </tr>  </tbody></table>"); 
// client.print("<br><div><a href=\"/Time\">Time</a></div>");
            // HTTP-ответ заканчивается еще одной пустой строкой:
            client.println("</html>");
            break;
          }
          else {      
            currentLine = "";
          }
        }
        else if (c != '\r') {    
          currentLine += c;     
        }
        // создание кнопок
        if (currentLine.endsWith("GET /R")) {
             LEDS.setBrightness(200);
        }
        if (currentLine.endsWith("GET /F")) {
              LEDS.setBrightness(150);
        }
        if (currentLine.endsWith("GET /L")) {
              LEDS.setBrightness(100);
        }
        if (currentLine.endsWith("GET /H")) {
              LEDS.setBrightness(50);
        }
        if (currentLine.endsWith("GET /Am")) {
              ledColor = CRGB::Amethyst;
        }
        if (currentLine.endsWith("GET /Aq")) {
              ledColor = CRGB::Aqua;
        }
        if (currentLine.endsWith("GET /RED")) {
              ledColor = CRGB::Red;
        }
        if (currentLine.endsWith("GET /C")) {
              ledColor = CRGB::Crimson;
        }
        if (currentLine.endsWith("GET /DM")) {
              ledColor = CRGB::DarkMagenta;
        }
        if (currentLine.endsWith("GET /DP")) {
              ledColor = CRGB::DeepPink;
        }
        if (currentLine.endsWith("GET /CY")) {
              ledColor = CRGB::Cyan;
        }
        if (currentLine.endsWith("GET /DGr")) {
              ledColor = CRGB::DarkGoldenrod;
        }
        if (currentLine.endsWith("GET /MB")) {
              ledColor = CRGB::MidnightBlue;
        }
        if (currentLine.endsWith("GET /Green")) {
              ledColor = CRGB::DarkGreen;
        }
        if (currentLine.endsWith("GET /N")) {
              ledColor = CRGB::Navy;
        }
        if (currentLine.endsWith("GET /Ch")) {
              ledColor = CRGB::Chocolate;
        }
        if (currentLine.endsWith("GET /O")) {
              ledColor = CRGB::OrangeRed;
        }
        if (currentLine.endsWith("GET /LM")) {
              ledColor = CRGB::Lime;
        }
        if (currentLine.endsWith("GET /RB")) {
              ledColor = CRGB::RoyalBlue;
        }
        if (currentLine.endsWith("GET /T")) {
              ledColor = CRGB::Tomato;
        }

      }
    }
    // закрыть соединение
    client.stop();
    // Serial.println("client disconnected");
  }
}


//* Посылаем запрос NTP серверу на заданный адрес *//
unsigned long sendNTPpacket(IPAddress& address)
{
  // Serial.println("sending NTP packet...");
// Очистка буфера в 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
// Формируем строку зыпроса NTP сервера
  packetBuffer[0] = 0b11100011;   
  packetBuffer[1] = 0;    
  packetBuffer[2] = 6;     
  packetBuffer[3] = 0xEC;  
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
// Посылаем запрос на NTP сервер (123 порт)
  udp.beginPacket(address, 123); 
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}

/* Посылаем и парсим запрос к NTP серверу */
bool GetNTP(void) {
  WiFi.hostByName(ntpServerName, timeServerIP); 
  // Serial.print("ntpServerName :");Serial.print(ntpServerName);  Serial.print(" timeServerIP :");Serial.println(timeServerIP);
  sendNTPpacket(timeServerIP); 
  delay(1000);
  
  int cb = udp.parsePacket();
  if (!cb) {
    // Serial.println("No packet yet");
    return false;
  }
  else {
    // Serial.print("packet received, length=");
    // Serial.println(cb);

    udp.read(packetBuffer, NTP_PACKET_SIZE); // Читаем пакет в буфер   
// 4 байта начиная с 40-го сождержат таймстамп времени - число секунд 
// от 01.01.1900   
    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
// Конвертируем два слова в переменную long
    unsigned long secsSince1900 = highWord << 16 | lowWord;
// Конвертируем в UNIX-таймстамп (число секунд от 01.01.1970)
    const unsigned long seventyYears = 2208988800UL;
    unsigned long epoch = secsSince1900 - seventyYears;
// Делаем поправку на местную тайм-зону
      epoch = epoch + TIMEZONE*3600;   
    // Serial.print("Unix time = ");
    // Serial.println(epoch);

   uint16_t s2=epoch%60; epoch/=60; // минута в данный момент
   uint16_t m2=epoch%60; epoch/=60; // час в данный момент
   uint16_t h2=epoch%24; epoch/=24; // день в данный момент
   uint16_t w2=(epoch+4)%7;         // день недели

   uint16_t y2=70; 
   int d2=0;
   while(d2 + ((y2 % 4) ? 365 : 366) <= epoch) { d2 += (y2 % 4) ? 365 : 366;  y2++; }
   y2 = y2 + 1900;
   epoch -= d2; // теперь дни в этом году, начинающиеся с 0
//   d2=0;

   int  mt2=0;
   byte monthLength=0;
   for (mt2=0; mt2<12; mt2++) {
    if (mt2==1) { 
      if (y2%4) monthLength=28;
      else monthLength=29;
    }
    else monthLength = monthDays[mt2];
    if (epoch>=monthLength) epoch-=monthLength;
    else break;   
    }
    mt2++;       
    uint16_t d3=epoch+1; 
    
  // записываем в RTC текущее время
   DateTime now;
      now.second = s2;
      now.minute = m2;
      now.hour = h2;
      now.date = d3;
      now.month = mt2;
      now.year = y2;
      
      rtc.setTime(now);  // загружаем в RTC
  
  // Serial.print("Time: "); Serial.print(h2); Serial.print(":"); Serial.print(m2);  Serial.print(":"); Serial.println(s2);
  // Serial.print("Date: "); Serial.print(d3); Serial.print(":"); Serial.print(mt2); Serial.print(":"); Serial.println(y2);
  }
  return true;
}


// Получение времени в виде одного числа(15:45:20 = 154520)
uint32_t GetTime() {
  
  DateTime now = rtc.getTime();

  // Serial.print(now.hour, DEC);
  // Serial.print(':');
  // Serial.print(now.minute,DEC);
  // Serial.print(':');
  // Serial.println(now.second,DEC);

  uint32_t hour = now.hour;
  uint32_t minute = now.minute;
  uint32_t second = now.second;

  return (hour * 10000 + minute * 100 + second);
};

// преобразование времени в массив, необходимый для отображения
void TimeToArray() {
  uint32_t myTime = GetTime();  //  Получить текущее время

  int cursor = 88; // номер последнего светодиода

  // Установка какие светодиоды должны гореть, если точки включены
  if (Dot) {
    leds[59] = ledColor;
    leds[58] = ledColor;
    leds[29] = ledColor;
    leds[28] = ledColor;
  }
  else  {
    leds[59] = 0x000000;
    leds[58] = 0x000000;
    leds[29] = 0x000000;
    leds[28] = 0x000000;
  };

  for (int i = 1; i <= 6; i++) {
    uint32_t digit = myTime % 10; // получить последнюю цифру
if (i == 1) {
//        Serial.print("Digit 6 is : ");Serial.print(digit);Serial.print(" ");
      cursor = 74;

      for (int k = 0; k <= 13; k++) {
//         Serial.print(digits[digit][k]);
        if (digits[digit][k] == 1) {
          leds[cursor] = ledColor;
        }
        else if (digits[digit][k] == 0) {
          leds[cursor] = 0x000000;
        };
        cursor ++;
      };
//       Serial.println();
      

    }
    
else if (i == 2) {
//        Serial.print("Digit 5 is : ");Serial.print(digit);Serial.print(" ");
      cursor = 60;

      for (int k = 0; k <= 13; k++) {
//         Serial.print(digits[digit][k]);
        if (digits[digit][k] == 1) {
          leds[cursor] = ledColor;
        }
        else if (digits[digit][k] == 0) {
          leds[cursor] = 0x000000;
        };
        cursor ++;
      };
//   Serial.println();
     }
   
if (i == 3) {
//        Serial.print("Digit 4 is : ");Serial.print(digit);Serial.print(" ");
      cursor = 44;

      for (int k = 0; k <= 13; k++) {
//         Serial.print(digits[digit][k]);
        if (digits[digit][k] == 1) {
          leds[cursor] = ledColor;
        }
        else if (digits[digit][k] == 0) {
          leds[cursor] = 0x000000;
        };
        cursor ++;
      };
//       Serial.println();
    // когда цифра минуты сменяется запускать эффект cylon
    if (digit != last_digit)
        {
          cylon();
        }
        last_digit = digit;
      }
      
  else if (i == 4) {
//       Serial.print("Digit 3 is : ");Serial.print(digit);Serial.print(" ");
      cursor = 30;

      for (int k = 0; k <= 13; k++) {
//         Serial.print(digits[digit][k]);
        if (digits[digit][k] == 1) {
          leds[cursor] = ledColor;
        }
        else if (digits[digit][k] == 0) {
          leds[cursor] = 0x000000;
        };
        cursor ++;
      };
//       Serial.println();
    }
if (i == 5) {
//       Serial.print("Digit 2 is : ");Serial.print(digit);Serial.print(" ");
      cursor = 14;
      for (int k = 0; k <= 13; k++) {
//         Serial.print(digits[digit][k]);
        if (digits[digit][k] == 1) {
          leds[cursor] = ledColor;
        }
        else if (digits[digit][k] == 0) {
          leds[cursor] = 0x000000;
        };
        cursor ++;
      };
//       Serial.println();
    }
 else if (i == 6) {
//       Serial.print("Digit 1 is : ");Serial.print(digit);Serial.print(" ");
      cursor = 0;
      for (int k = 0; k <= 13; k++) {
//         Serial.print(digits[digit][k]);
        if (digits[digit][k] == 1) {
          leds[cursor] = ledColor;
        }
        else if (digits[digit][k] == 0) {
          leds[cursor] = 0x000000;
        };
        cursor ++;
      };
//       Serial.println();
    }
    myTime /= 10;
  };
};


// преобразование температуры в массив, необходимый для отображения
void TempToArray() {
  DateTime now = rtc.getTime();
  if(now.second != 17) {
    TempShow = false;
    return;
  }
 TempShow = true;
 int32_t t = dht.readTemperature();
 int32_t celsius = t;
 if (celsius <= -1){
   celsius = t * -100;
 }
else if (celsius >= 0){
  celsius = t * 100;
} 
int32_t minus = t;
//  Serial.println(t);
//  Serial.println(minus);
//    Serial.print("Temp is: ");Serial.println(celsius);

 int cursor = 88; // последний номер светодиода

  leds[59] = 0x000000;
  leds[58] = 0x000000;
  leds[29] = 0x000000;
  leds[28] = 0x000000;

 for (int i = 1; i <= 6; i++) {
   int32_t digit = celsius % 10; 
   if (i == 1) {
     // Serial.print("Digit 6 is : ");Serial.print(digit);Serial.print(" ");
     cursor = 74;

     for (int k = 0; k <= 13; k++) {
       //  Serial.print(digits[11][k]);
       if (digits[11][k] == 1) {
         leds[cursor] = ledColor;
       }
       else if (digits[11][k] == 0) {
         leds[cursor] = 0x000000;
       };
       cursor ++;
     };
     //  Serial.println();
   }
   else if (i == 2) {
     //  Serial.print("Digit 5 is : ");Serial.print(digit);Serial.print(" ");
     cursor = 60;

     for (int k = 0; k <= 13; k++) {
       //   Serial.print(digits[10][k]);
       if (digits[10][k] == 1) {
         leds[cursor] = ledColor;
       }
       else if (digits[10][k] == 0) {
         leds[cursor] = 0x000000;
       };
       cursor ++;
     };
     //  Serial.println();
   }
  if (i == 3) {
     //  Serial.print("Digit 4 is : ");Serial.print(digit);Serial.print(" ");
     cursor = 44;
     for (int k = 0; k <= 13; k++) {
       //  Serial.print(digits[digit][k]);
       if (digits[digit][k] == 1) {
         leds[cursor] = ledColor;
       }
       else if (digits[digit][k] == 0) {
         leds[cursor] = 0x000000;
       };
       cursor ++;
     };
     // Serial.println();
   }
  else if (i == 4) {
     //  Serial.print("Digit 3 is : ");Serial.print(digit);Serial.print(" ");
     cursor = 30;
     for (int k = 0; k <= 13; k++) {
       //   Serial.print(digits[digit][k]);
       if (digits[digit][k] == 1) {
         leds[cursor] = ledColor;
       }
       else if (digits[digit][k] == 0) {
         leds[cursor] = 0x000000;
       };
       cursor ++;
     };
     //  Serial.println();
   }
  if (i == 5) {
      // Serial.print("Digit 2 is : ");Serial.print(digit);Serial.print(" ");
     cursor = 14;
     for (int k = 0; k <= 13; k++) {
        //  Serial.print(digits[13][k]);
        if (minus <= -1){
        if (digits[13][k] == 1) {
         leds[cursor] = ledColor;
       }
       else if (digits[13][k] == 0) {
         leds[cursor] = 0x000000;
       };
        }
        else if (minus >=0){
           if (digits[12][k] == 1) {
         leds[cursor] = ledColor;
       }
       else if (digits[12][k] == 0) {
         leds[cursor] = 0x000000;
       };
        };
       cursor ++;
     };
      // Serial.println();
   }
   else if (i == 6) {
     //  Serial.print("Digit 1 is : ");Serial.print(digit);Serial.print(" ");
     cursor = 0;
     for (int k = 0; k <= 13; k++) {
       //   Serial.print(digits[digit][k]);
       if (digits[12][k] == 1) {
         leds[cursor] = ledColor;
       }
       else if (digits[12][k] == 0) {
         leds[cursor] = 0x000000;
       };
       cursor ++;
     };
     //  Serial.println();
   }
   celsius /= 10;
 };
};

 //функция перехода минут//

void fadeall() {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i].nscale8(250);
  }
}
// цикличное изменение цветов
void cylon () {
  static uint8_t hue = 0;
  // сначала сдвигается светодиод в одном направлении
  for (int i = 0; i < NUM_LEDS; i++) {
    // Установите i светодиод на красный
    leds[i] = CHSV(hue++, 255, 255);
    // вывести на ленту
    FastLED.show();
    fadeall();
    // подождать немного, прежде чем сделать круг и повторяем цикл
    delay(10);
  }
  // Теперь в другом направлении
  for (int i = (NUM_LEDS) - 1; i >= 0; i--) {
    // Установите i светодиод на красный
    leds[i] = CHSV(hue++, 255, 255);
    // вывести на ленту
    FastLED.show();
    fadeall();
    // подождать немного, прежде чем сделать круг и повторяем цикл
    delay(9);
  }
}

// Основной цикл
void loop()  {
   // Считывание времени  
  cur_ms       = millis();
  if( cur_ms < ms2 || (cur_ms - ms2) > 604800000){   // Каждые 7 суток считываем время в интернете 
      err_count++;
      connect_WiFi();
      if (WiFi.status() == WL_CONNECTED) { 
      // Serial.println("WIFI Get NTP time.");
      if( GetNTP() ){
         ms2       = cur_ms;
         err_count = 0;
        // Serial.println("Time is counted");
         }
        CreateAccessPoint();         
        server.begin();         
       }
   }
  
  printWEB();
  TimeToArray(); 
  TempToArray();   
  FastLED.show(); 
    if (TempShow == true) delay (5000); // время отображение температуры 5 секунд

}
