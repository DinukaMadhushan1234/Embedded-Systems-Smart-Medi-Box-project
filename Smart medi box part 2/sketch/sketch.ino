#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHTesp.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>
#include <NTPClient.h>
#include <WiFiUdp.h>


//OLED display
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0X3C

//Input and output pins
#define BUZZER 5
#define LED_1 15
#define LED_2 2
#define PB_CANCEL 34
#define PB_OK 32
#define PB_UP 13
#define PB_DOWN 35
#define DHTPIN 12
#define LDR_L A0
#define LDR_R 33
#define SERVMO 18

//Parameters related to light intensity
float controlling_factor = 0.75;
const float RL10 = 50;
float MIN_ANGLE = 30;
float Max_intensity = 0;

float D_servo;

//time initialization
#define NTP_SERVER     "pool.ntp.org"
int UTC_OFFSET = 5*60*60 + 30*60;;
#define UTC_OFFSET_DST 0
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//Wifi and mqtt clients
WiFiClient espClient;
PubSubClient mqttClient(espClient);


// to store light and temperature data to send through mqtt
char tempAr[10];
char lightAr[10];
char Max_light_direction[10];

DHTesp dhtSensor;

int days = 0;
int hours = 0;
int minutes = 0;
int seconds = 0;

unsigned long timeNow = 0;
unsigned long timeLast = 0;

bool isScheduledON = false;
unsigned long scheduledOnTime;

bool alarm_enabled = true;
int n_alarms = 3;
int alarm_hours[] = {0, 1, 2};
int alarm_minutes[] = {1, 10, 25};
bool alarm_triggered[] = {false, false, false};

int n_notes = 8;
int C = 262;
int D = 294;
int E = 330;
int F = 349;
int G = 392;
int A = 440;
int B = 494;
int C_H = 523;
int notes[] = {C, D, E, F, G, A, B, C_H};

int current_mode = 0;
int max_modes = 5;
String modes[] = {"1 - Set Time", "2 - Set Alarm 1", "3 - Set Alarm 2", "4 - Set Alarm 3","5 - Disable Alarms"};

//servo moter initializing position
int pos = 0;
Servo servo;

void setup() {
  pinMode(BUZZER, OUTPUT);
  pinMode(LED_1, OUTPUT);
  pinMode(LED_2, OUTPUT);
  pinMode(PB_CANCEL, INPUT);
  pinMode(PB_OK, INPUT);
  pinMode(PB_UP, INPUT);
  pinMode(PB_DOWN, INPUT);
  pinMode(LDR_L, INPUT);
  pinMode(LDR_R, INPUT);

  dhtSensor.setup(DHTPIN, DHTesp::DHT22);
  servo.attach(SERVMO, 500, 2400);

  Serial.begin(115200);
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }

  display.display();
  delay(2000);


  setupWifi();
  setupMqtt();

  timeClient.begin();
  timeClient.setTimeOffset(5.5*3600);
  configTime(UTC_OFFSET, UTC_OFFSET_DST, NTP_SERVER);

  display.clearDisplay();

  print_line("Welcome", 25, 0, 2);
  print_line("to", 50, 20, 2);
  print_line("MediBox!", 20, 40, 2);
  display.clearDisplay();
}

void loop() {
  //connect with mqtt broker
  if(!mqttClient.connected()){
    connectToBroker();
  }
  mqttClient.loop();
  //publish topics with data
  mqttClient.publish("ENTC-ADMIN-LIGHT", lightAr);
  mqttClient.publish("ENTC-ADMIN-TEMP", tempAr);
  mqttClient.publish("ENTC-ADMIN-LIGHT-DIRECTION", Max_light_direction);
  
  update_time_with_check_alarm();
  if (digitalRead(PB_OK) == LOW) {
    delay(200);
    go_to_menu();
  }
  check_temp();
  read_ldr() ;
  serv_mo();
  check_schedule();
}

void connectToBroker() {
  while(!mqttClient.connected()){
    Serial.println("Attempting MQTT connection");
    if(mqttClient.connect("ESP32-12345645454")){
      Serial.println("connected");
      mqttClient.subscribe("ENTC-ADMIN-ANGLE");
      mqttClient.subscribe("ENTC-ADMIN-CONT-FACTOR");
      mqttClient.subscribe("ENTC-ADMIN-ON-OFF");
      mqttClient.subscribe("ENTC-ADMIN-SCH-ON");
    }else{
      Serial.println("failed");
      Serial.println(mqttClient.state());
      delay(5000);
    }
  }
}

void setupMqtt() {
  mqttClient.setServer("test.mosquitto.org", 1883);
  mqttClient.setCallback(recieveCallback);
}

void recieveCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  char payloadCharAr[length];
  for (int i = 0; i < length; i++){
    Serial.print((char)payload[i]);
    payloadCharAr[i] = (char)payload[i];
  }
  Serial.print("\n");
 
  //receive minimum angle 
  if (strcmp(topic, "ENTC-ADMIN-ANGLE") == 0){
    MIN_ANGLE = atoi(payloadCharAr);
  }
  //receive control factor
  if (strcmp(topic, "ENTC-ADMIN-CONT-FACTOR") == 0) {
    controlling_factor = atof(payloadCharAr);
  }
  //receive main switch status
  if (strcmp(topic, "ENTC-ADMIN-ON-OFF") == 0) {
    buzzerOn(payloadCharAr[0] == 't');
  }
  //receive scheduled time
  if(strcmp(topic, "ENTC-ADMIN-SCH-ON") == 0){
    if(payloadCharAr[0] =='N'){
      isScheduledON = false;
    }else{
      isScheduledON = true;
      scheduledOnTime = atol(payloadCharAr);
    }
  }


}


void setupWifi() {
  WiFi.begin("Wokwi-GUEST", "", 6);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    display.clearDisplay();
    print_line("Connecting to WiFi", 0, 0, 2);
  }

  display.clearDisplay();
  print_line("Connected to WiFi", 0, 0, 2);
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  delay(500);
}

void buzzerOn(bool on){
  if(on){
    tone(BUZZER, C);
  }
  else{
    noTone(BUZZER);
  }
}

unsigned long getTime(){
  timeClient.update();
  return timeClient.getEpochTime();
}

void read_ldr() {
  float lightIntensityLeft;
  float lightIntensityRight;
  String direction;

  lightIntensityLeft = (4095 - analogRead(LDR_L)) / 4095.00; 
  lightIntensityRight = (4095 - analogRead(LDR_R)) / 4095.00;

  if(lightIntensityLeft > lightIntensityRight){
    Max_intensity = lightIntensityLeft;
    direction = "Left";
    D_servo = 1.5;
    Serial.print(lightIntensityLeft);
    Serial.println(" - Left LDR");
  }
  else if(lightIntensityLeft < lightIntensityRight){
    Max_intensity = lightIntensityRight;
    direction = "Right";
    D_servo = 0.5;
    Serial.println(lightIntensityRight);
    Serial.println(" - Right LDR");
  }
  else{
    Max_intensity = lightIntensityRight;
    direction = "Both equal";
    Serial.println(lightIntensityRight);
    Serial.println(" - Both LDRs");
  }

  String(direction).toCharArray(Max_light_direction,6);
  String(Max_intensity).toCharArray(lightAr,6);

}

void serv_mo(){
  //calculating position of servo motor
  pos = static_cast<int>(min(static_cast<float>(MIN_ANGLE) * D_servo + (180 - MIN_ANGLE) * Max_intensity * controlling_factor, 180.0f));
  servo.write(pos);
}

void check_schedule() {
  // set the buzzer to tone when scheduled time comes
  if (isScheduledON){
    unsigned long currentTime = getTime();
    if (currentTime > scheduledOnTime) {
      buzzerOn(true);
      isScheduledON = false;
      mqttClient.publish("ENTC-ADMIN-ON-OFF-ESP", "1");
      mqttClient.publish("ENTC-ADMIN-ON-OFF-SCH", "0");
      Serial.println("Scheduled ON");
      Serial.println(currentTime);
      Serial.println(scheduledOnTime);
    }
  }
}

//End of code snippets related to Assignment 2
//===============================================================

void print_line(String text, int column, int row, int text_size) {

  display.setTextSize(text_size);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(column, row);
  display.println(text);
  display.display();

}

void print_time_now(void) {
  display.clearDisplay();
  print_line("Day:",0, 0, 2);
  print_line(String(days), 50, 0, 2);
  print_line(String(hours), 0, 20, 2);
  print_line(":", 20, 20, 2);
  print_line(String(minutes), 30, 20, 2);
  print_line(":", 50, 20, 2);
  print_line(String(seconds), 60, 20, 2);

}

void update_time() {
  struct tm timeinfo;
  getLocalTime(&timeinfo);

  char timeHour[3];
  strftime(timeHour, 3, "%H", &timeinfo);
  hours = atoi(timeHour);

  char timeMinute[3];
  strftime(timeMinute, 3, "%M", &timeinfo);
  minutes = atoi(timeMinute);

  char timeSecond[3];
  strftime(timeSecond, 3, "%S", &timeinfo);
  seconds = atoi(timeSecond);

  char timeDay[3];
  strftime(timeDay, 3, "%d", &timeinfo);
  days = atoi(timeDay);

}

void ring_alarm() {
  display.clearDisplay();
  print_line("MEDICINE TIME!", 0, 0, 2);
  digitalWrite(LED_1, HIGH);

  bool break_happened = false;
  while (break_happened == false && digitalRead(PB_CANCEL) == HIGH) {
    for (int i = 0; i < n_notes; i++) {
      if (digitalRead(PB_CANCEL) == LOW) {
        delay(200);
        break_happened = true;
        break;
      }
      tone(BUZZER, notes[i]);
      delay(500);
      noTone(BUZZER);
      delay(2);
    }
  }

  digitalWrite(LED_1, LOW);
  display.clearDisplay();
}

void update_time_with_check_alarm() {
  update_time();
  print_time_now();

  if (alarm_enabled == true) {
    for (int i = 0; i < n_alarms; i++) {
      if (alarm_triggered[i] == false && alarm_hours[i] == hours && alarm_minutes[i] == minutes) {
        ring_alarm();
        alarm_triggered[i] = true;
      }
    }
  }
}

int wait_for_button_press() {
  while (true) {
    if (digitalRead(PB_UP) == LOW) {
      delay(200);
      return PB_UP;
    }
    else if (digitalRead(PB_DOWN) == LOW) {
      delay(200);
      return PB_DOWN;
    }
    else if (digitalRead(PB_OK) == LOW) {
      delay(200);
      return PB_OK;
    }
    else if (digitalRead(PB_CANCEL) == LOW) {
      delay(200);
      return PB_CANCEL;
    }
    update_time();
  }
}

void go_to_menu() {
  // display.clearDisplay();
  // print_line("MENU",0,0,2);
  // delay(1000);
  while (digitalRead(PB_CANCEL) == HIGH) {
    display.clearDisplay();
    print_line(modes[current_mode], 0, 0, 2);

    int pressed = wait_for_button_press();
    if (pressed == PB_UP) {
      delay(200);
      current_mode += 1;
      current_mode = current_mode % max_modes;
    }

    else if (pressed == PB_DOWN) {
      delay(200);
      current_mode -= 1;
      if (current_mode < 0) {
        current_mode = max_modes - 1;
      }
    }
    else if (pressed == PB_OK) {
      delay(200);
      Serial.println(current_mode);
      run_mode(current_mode);
    }
    else if (pressed == PB_CANCEL) {
      delay(200);
      break;
    }
  }
}

void set_time() {
  int temp_hour = 5;

  while (true) {
    display.clearDisplay();
    print_line("Hour offset:" + String(temp_hour), 0, 0, 2);

    int pressed = wait_for_button_press();
    if (pressed == PB_UP) {
      delay(200);
      temp_hour += 1;
      if (temp_hour > 14) {
        temp_hour = -12;
      }
    }

    else if (pressed == PB_DOWN) {
      delay(200);
      temp_hour -= 1;
      if (temp_hour < -12) {
        temp_hour = 14;
      }
    }
    else if (pressed == PB_OK) {
      delay(200);
      hours = temp_hour;
      break;
    }
    else if (pressed == PB_CANCEL) {
      delay(200);
      break;
    }
  }

  int temp_minute = 30;

  while (true) {
    display.clearDisplay();
    print_line("Minute offset:" + String(temp_minute), 0, 0, 2);

    int pressed = wait_for_button_press();
    if (pressed == PB_UP) {
      delay(200);
      temp_minute += 15;
      temp_minute = temp_minute % 60;
    }

    else if (pressed == PB_DOWN) {
      delay(200);
      temp_minute -= 15;
      if (temp_minute < 0) {
        temp_minute = 45;
      }
    }
    else if (pressed == PB_OK) {
      delay(200);
      minutes = temp_minute;
      if (temp_hour >= 0){
        UTC_OFFSET = temp_hour*60*60 + temp_minute*60;
      }
      else{
        UTC_OFFSET = -(-temp_hour*60*60 + temp_minute*60);
      }
      configTime(UTC_OFFSET, UTC_OFFSET_DST, NTP_SERVER);
      update_time();  
      break;
    }
    else if (pressed == PB_CANCEL) {
      delay(200);
      break;
    }
  }

  display.clearDisplay();
  print_line("Time is  set", 0, 0, 2);
  delay(1000);

}

void set_alarm(int alarm) {
  int temp_hour = alarm_hours[alarm];

  while (true) {
    display.clearDisplay();
    print_line("Enter hour:" + String(temp_hour), 0, 0, 2);

    int pressed = wait_for_button_press();
    if (pressed == PB_UP) {
      delay(200);
      temp_hour += 1;
      temp_hour = temp_hour % 24;
    }

    else if (pressed == PB_DOWN) {
      delay(200);
      temp_hour -= 1;
      if (temp_hour < 0) {
        temp_hour = 23;
      }
    }
    else if (pressed == PB_OK) {
      delay(200);
      alarm_hours[alarm] = temp_hour;
      break;
    }
    else if (pressed == PB_CANCEL) {
      delay(200);
      break;
    }
  }

  int temp_minute = alarm_minutes[alarm];

  while (true) {
    display.clearDisplay();
    print_line("Enter minute:" + String(temp_minute), 0, 0, 2);

    int pressed = wait_for_button_press();
    if (pressed == PB_UP) {
      delay(200);
      temp_minute += 1;
      temp_minute = temp_minute % 60;
    }

    else if (pressed == PB_DOWN) {
      delay(200);
      temp_minute -= 1;
      if (temp_minute < 0) {
        temp_minute = 59;
      }
    }
    else if (pressed == PB_OK) {
      delay(200);
      alarm_minutes[alarm] = temp_minute;
      break;
    }
    else if (pressed == PB_CANCEL) {
      delay(200);
      break;
    }
  }

  display.clearDisplay();
  print_line("Alarm is  set", 0, 0, 2);
  delay(1000);

}

void run_mode(int mode) {
  if (mode == 0) {
    set_time();
  }

  else if (mode == 1 || mode == 2 || mode == 3) {
    set_alarm(mode - 1);
  }

  else if (mode == 4) {
    alarm_enabled = false;
    print_line("All alarms are disabled", 0, 0, 2);
    delay(1000);
  }
}

// void check_temp() {
//   TempAndHumidity data = dhtSensor.getTempAndHumidity();
//   if (data.temperature > 35) {
//     display.clearDisplay();
//     print_line("TEMP HIGH", 0, 40, 1);
//   }
//   else if (data.temperature < 25) {
//     display.clearDisplay();
//     print_line("TEMP LOW", 0, 40, 1);
//   }

//   if (data.humidity > 40) {
//     display.clearDisplay();
//     print_line("HUMIDITY HIGH", 0, 50, 1);
//   }
//   else if (data.humidity < 20) {
//     display.clearDisplay();
//     print_line("HUMIDITY LOW", 0, 50, 1);
//   }

//     String(data.temperature,2).toCharArray(tempAr, 10);
// }

void check_temp(void) {
  TempAndHumidity data = dhtSensor.getTempAndHumidity(); //Read temperature and humidity from sensor
  bool all_good = true;

  //Give warnings if healthy limits exceeded
  if (data.temperature > 32) {
    all_good = false;
    digitalWrite(LED_2, HIGH);
    print_line("Temperature High", 1, 40, 0);
  }

  else if (data.temperature < 26) {
    all_good = false;
    digitalWrite(LED_2, HIGH);
    print_line("Temperature Low", 1, 40, 0);
  }

  if (data.humidity > 80) {
    all_good = false;
    digitalWrite(LED_2, HIGH);
    print_line("Humidity High", 1, 50, 0);
  }

  if (data.humidity < 60) {
    all_good = false;
    digitalWrite(LED_2, HIGH);
    print_line("Humidity Low", 1, 50, 0);
  }

  if (all_good) {
    digitalWrite(LED_2, LOW);
  }
  else {
      tone(BUZZER, G);
      delay(600);
      noTone(BUZZER);
      delay(2);
  }

  String(data.temperature,2).toCharArray(tempAr, 10);
}