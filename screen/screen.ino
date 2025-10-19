#include "Network.h"
#include "inkscreen.h"

#include "WiFi.h"
#include "ArduinoJson.h"
#include "HTTPClient.h"
#include "string"
#include "stdint.h"
#include "malloc.h"

// 按键高电平触发
#define KEY0 2
#define KEY1 13
// LED低电平亮
#define LED_R 27
#define LED_G 26
#define LED_B 25
// 墨水屏引脚
#define SCR_DC 23
#define SCR_RST 18
#define SCR_MOSI 21
#define SCR_CLK 22
#define SCR_CS 19
#define SCR_BUSY 5

// 生成状态
typedef enum{
  GENERATE_STATE_START = 0,  // 开始生成
  GENERATE_STATE_RESPONSE_200,  // 已收到httpcode=200
  GENERATE_STATE_DOWNLOAD,  // 生图执行正常, 即将下载
  GENERATE_STATE_SUCCESS,  // 完成
  GENERATE_STATE_ERROR = -1  // 错误
}GENERATE_STATE;

Network network;
InkScreen inkscreen(SCR_MOSI, SCR_CLK, SCR_CS, SCR_DC, SCR_RST, SCR_BUSY);
uint8_t* buffer;
HTTPClient http;
DynamicJsonDocument doc(40);
int httpCode = 0;
GENERATE_STATE generate_state = GENERATE_STATE_START;


uint32_t lastInterruptTime = 0;
uint32_t lastErrTime = 0;

void IRAM_ATTR handleKey0Interrupt() {
  if (millis() - lastInterruptTime > 200) {
    network.SetConnectMode();
  }
  lastInterruptTime = millis();
}

void IRAM_ATTR handleKey1Interrupt() {
  if (millis() - lastInterruptTime > 200) {
    network.SetConfigMode();
  }
  lastInterruptTime = millis();
}

void EnterDeepSleep(uint32_t sleepMinutes) {
  // 清除之前的唤醒配置
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_EXT1);
  
  // 配置EXT1唤醒（按键唤醒）- 任意按键高电平唤醒
  uint64_t wakeupPins = (1ULL << KEY0) | (1ULL << KEY1);
  esp_sleep_enable_ext1_wakeup(wakeupPins, ESP_EXT1_WAKEUP_ANY_HIGH);
  
  // 配置定时器唤醒
  if (sleepMinutes > 0) {
    uint64_t sleepTimeUs = sleepMinutes * 60000000ULL;
    esp_sleep_enable_timer_wakeup(sleepTimeUs);
  }
  
  // 关闭外设以省电
  digitalWrite(LED_R, 1);
  digitalWrite(LED_G, 1);
  digitalWrite(LED_B, 1);
  http.end();
  Serial.flush();
  
  // 进入深度睡眠
  esp_deep_sleep_start();
}

void Generate(){
  if(generate_state == GENERATE_STATE_START){
    // 生图
    String img_url = "https://15252f15adb1fca493b9b25d11c4468f.apig.cn-north-4.huaweicloudapis.com/image_generation";
    http.setTimeout(30000);
    http.begin(img_url.c_str());
    
    // 设置请求体参数
    String jsonBody = "{";
        jsonBody += "\"llm_model\":\"" + network.llm_model + "\",";
        jsonBody += "\"llm_url\":\"" + network.llm_url + "\",";
        jsonBody += "\"llm_key\":\"" + network.llm_key + "\",";
        jsonBody += "\"img_key\":\"" + network.img_key + "\"";
        jsonBody += "}";
        
    Serial.println("Request");
    httpCode = http.POST(jsonBody);
    Serial.println(httpCode);  

    String payload = http.getString();  // 获取响应的内容
    Serial.println("Response:");
    Serial.println(payload);

    if (httpCode != 200) {  // 检查响应码
      http.end();  // 关闭HTTP连接
      httpCode = 0;
      generate_state = GENERATE_STATE_ERROR;
      lastErrTime = millis();
      return;
    } else {
      deserializeJson(doc, payload);
      if(!doc["error_code"].isNull() || doc["status"].as<uint8_t>() != 0){  // 检验图片生成是否成功(status=0), 华为云错误(如超时)实际上httpcode=500
        http.end();  // 关闭HTTP连接
        httpCode = 0;
        generate_state = GENERATE_STATE_ERROR;
        lastErrTime = millis();
        return;
      } else {
        generate_state = GENERATE_STATE_RESPONSE_200;
        httpCode = 0;
      }
    }
  }

  if(generate_state == GENERATE_STATE_RESPONSE_200){
    // 下载图片
    httpCode = 0;
    char download_url[512] = "https://newbucket-345ljh.oss-cn-shenzhen.aliyuncs.com/a.img";
    http.begin(download_url);
    httpCode = http.GET();
    Serial.println(httpCode);
    if (httpCode > 0) {  // 检查响应码
      String response = http.getString();
      buffer = (uint8_t*)response.c_str();
    }

    inkscreen.SetImage(buffer);
    inkscreen.Sleep();
    delay(10000);
    generate_state = GENERATE_STATE_SUCCESS;
    EnterDeepSleep(30);
    esp_deep_sleep_start();
  }
}
void setup() {
  Serial.begin(115200);

  buffer = (uint8_t*)malloc(30000);
  memset(buffer, 0, 30000);

  network.Init();
  inkscreen.Init();
  http.setTimeout(30000);

  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);
  digitalWrite(LED_R, 0);
  digitalWrite(LED_G, 0);
  digitalWrite(LED_B, 0);

  pinMode(KEY0, INPUT_PULLDOWN);
  pinMode(KEY1, INPUT_PULLDOWN);
  attachInterrupt(digitalPinToInterrupt(KEY0), handleKey0Interrupt, RISING);
  attachInterrupt(digitalPinToInterrupt(KEY1), handleKey1Interrupt, RISING);

  // 获取唤醒原因
  esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();

  switch(cause){
    case ESP_SLEEP_WAKEUP_EXT1:
      {
        uint64_t wakeupPins = esp_sleep_get_ext1_wakeup_status();
        if (wakeupPins & (1ULL << KEY0)) {  // key0唤醒, 进入连接模式
          network.SetConnectMode();
        }
      
        if (wakeupPins & (1ULL << KEY1)) {  // key1唤醒, 进入配网模式
          network.SetConfigMode();
        }
        break;
      }
    case ESP_SLEEP_WAKEUP_TIMER: // 定时器唤醒, 进入连接模式
    default:
        network.SetConnectMode();
      break;
  }

  network.ConnectSaved();

}

void loop() {
  network.Loop();
  if(millis() > 300000LL){  // 运行时间>5min(如函数持续访问失败, API暂时达到用量上限)
      EnterDeepSleep(30);  // 30min
      esp_deep_sleep_start();
  }
  switch(network.GetConnectState()){
    case NETWORK_STATE_CONNECTED:
      if(generate_state == GENERATE_STATE_ERROR){
        digitalWrite(LED_R, 0);
        digitalWrite(LED_G, 1);
        digitalWrite(LED_B, 0);
        if(millis() - lastErrTime > 30000){  // 错误30s后重试
          generate_state = GENERATE_STATE_START;
        }
      } else {
        digitalWrite(LED_R, 1);
        digitalWrite(LED_G, 0);
        digitalWrite(LED_B, 1);
        Generate();
      }
      break;
    case NETWORK_STATE_CONNECT_ATTEMPTING:
      digitalWrite(LED_R, 0);
      digitalWrite(LED_G, 0);
      digitalWrite(LED_B, 1);
      break;
    case NETWORK_STATE_CONFIG:
      digitalWrite(LED_R, 1);
      digitalWrite(LED_G, 1);
      digitalWrite(LED_B, 0);
      break;
    case NETWORK_STATE_CONFIG_PREPARE:
      digitalWrite(LED_R, 1);
      digitalWrite(LED_G, 1);
      digitalWrite(LED_B, 0);
      break;
    case NETWORK_STATE_TIMEOUT:
      EnterDeepSleep(30);  // 30min
      esp_deep_sleep_start();
      break;
  }
  delay(10);
}