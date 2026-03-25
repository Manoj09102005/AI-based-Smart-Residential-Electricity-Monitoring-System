#define BLYNK_TEMPLATE_ID "TMPL3BQ_XlkCZ"
#define BLYNK_TEMPLATE_NAME "demosmartREM"
#define BLYNK_AUTH_TOKEN "YNQQI_-l3MTdCqHkvU8xNkqwt9CyU-Ao"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <PZEM004Tv30.h>
#include <time.h>

char ssid[] = "vetri";
char pass[] = "vetri   18";

BlynkTimer timer;
PZEM004Tv30 pzem(Serial2, 16, 17);

#define RELAY_PIN 23
#define BUTTON_PIN 4

float maxCurrentLimit = 12.0;
float billLimit = 100.0;

bool alertSent = false;
bool aiControlEnabled = true;

float startOfDayEnergy = 0;
float startOfMonthEnergy = 0;

float dailyUnits = 0;
float monthlyUnits = 0;

int lastDay = -1;

const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 19800;
const int daylightOffset_sec = 0;

// 🔥 DEMO MODE
float demoMultiplier = 30.0;
bool demoMode = true;

// 🔥 BILL FUNCTION
float calculateBill(float units)
{
  float bill = 0;
  float u = units - 0.1;

  if(u <= 0) return 0;

  if(u <= 0.1)
    bill = u * 2.35;
  else if(u <= 0.3)
    bill = (0.1 * 2.35) + (u - 0.1) * 4.70;
  else if(u <= 0.4)
    bill = (0.1 * 2.35) + (0.2 * 4.70) + (u - 0.3) * 6.30;
  else if(u <= 0.5)
    bill = (0.1 * 2.35) + (0.2 * 4.70) + (0.1 * 6.30) + (u - 0.4) * 8.40;
  else
    bill = (0.1 * 2.35) + (0.2 * 4.70) + (0.1 * 6.30) + (0.1 * 8.40) + (u - 0.5) * 9.45;

  return bill;
}

BLYNK_CONNECTED()
{
  Blynk.syncVirtual(V8, V9, V17, V16);
}

BLYNK_WRITE(V8) { startOfDayEnergy = param.asFloat(); }
BLYNK_WRITE(V9) { startOfMonthEnergy = param.asFloat(); }

BLYNK_WRITE(V4)
{
  digitalWrite(RELAY_PIN, param.asInt());
}

BLYNK_WRITE(V15)
{
  if(aiControlEnabled)
  {
    digitalWrite(RELAY_PIN, param.asInt());
  }
}

BLYNK_WRITE(V16)  // AI ENABLE TOGGLE
{
  aiControlEnabled = param.asInt();
}

BLYNK_WRITE(V17)
{
  demoMode = param.asInt();
}

void checkTimeReset(float energy)
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)) return;

  int currentDay = timeinfo.tm_mday;

  if(currentDay != lastDay)
  {
    startOfDayEnergy = energy;
    Blynk.virtualWrite(V8, startOfDayEnergy);
    lastDay = currentDay;
  }
}

void sendData()
{
  float voltage = pzem.voltage();
  float current = pzem.current();
  float power   = pzem.power();

  float rawEnergy = pzem.energy();
  float energy = demoMode ? rawEnergy * demoMultiplier : rawEnergy;

  if (isnan(voltage) || isnan(current)) return;

  checkTimeReset(energy);

  dailyUnits   = energy - startOfDayEnergy;
  monthlyUnits = energy - startOfMonthEnergy;

  if(dailyUnits < 0) dailyUnits = 0;
  if(monthlyUnits < 0) monthlyUnits = 0;

  float totalBill = calculateBill(energy);
  float dailyBill = calculateBill(dailyUnits);

  // 📊 SEND DATA
  Blynk.virtualWrite(V0, voltage);
  Blynk.virtualWrite(V1, current);
  Blynk.virtualWrite(V2, power);
  Blynk.virtualWrite(V3, energy);

  Blynk.virtualWrite(V5, totalBill);
  Blynk.virtualWrite(V6, dailyUnits);
  Blynk.virtualWrite(V7, dailyBill);
  Blynk.virtualWrite(V10, monthlyUnits);

  // 🤖 AI PREDICTION
  float hoursPassed = millis() / 3600000.0;
  if(hoursPassed > 0)
  {
    float predictedDailyUsage = dailyUnits * (24.0 / hoursPassed);

    if(predictedDailyUsage > 2.0)
    {
      Blynk.logEvent("ai_warning", "⚠ AI: High usage predicted today!");
    }
  }

  // 🤖 AI SUGGESTION
  if(power > 2000)
  {
    Blynk.virtualWrite(V11, "AI: Turn OFF heavy appliances ⚡");
  }
  else
  {
    Blynk.virtualWrite(V11, "AI: Usage Normal ✅");
  }

  // ⚠ BILL ALERT
  if(totalBill > billLimit && !alertSent)
  {
    Blynk.logEvent("bill_alert", "⚠ Bill exceeded ₹20!");
    alertSent = true;
  }

  if(totalBill < billLimit)
    alertSent = false;

  // ⚡ AUTO CUT OFF
  if(current > maxCurrentLimit && aiControlEnabled)
  {
    digitalWrite(RELAY_PIN, LOW);
  }
}

void setup()
{
  Serial.begin(115200);

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  digitalWrite(RELAY_PIN, LOW);

  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED)
    delay(500);

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  timer.setInterval(4000L, sendData);
}

void loop()
{
  Blynk.run();
  timer.run();

  static unsigned long lastPress = 0;

  if(digitalRead(BUTTON_PIN) == LOW)
  {
    if(millis() - lastPress > 300)
    {
      digitalWrite(RELAY_PIN, !digitalRead(RELAY_PIN));
      lastPress = millis();
    }
  }
}