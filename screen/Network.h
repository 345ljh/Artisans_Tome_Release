#ifndef _NETWORK_H
#define _NETWORK_H

#define MAX_NETWORK 10

#include <WiFi.h>
#include <Preferences.h>
#include <WebServer.h>

typedef enum {
  NETWORK_STATE_CONNECTED = 0, // 连接成功
  NETWORK_STATE_INIT, // 模块初始化
  NETWORK_STATE_CONNECT_ATTEMPTING, // 尝试连接已储存网络
  NETWORK_STATE_CONFIG,  // 配网模式
  NETWORK_STATE_CONFIG_PREPARE,  // 准备进入配网模式
  NETWORK_STATE_TIMEOUT  // 连接/配网超时, 准备休眠
} NETWORK_STATE;

class Network: WebServer{
private:
  static Network* instance;
  Preferences prefs;
  // 记录nvs储存的网络
  String ssid[MAX_NETWORK];
  String password[MAX_NETWORK];
  // 连接情况
  NETWORK_STATE connectState = NETWORK_STATE_INIT;
  // 连接模式
  long connect_attempt_time = 0;  // 上一次尝试连接/进入配网的时间
  uint8_t attempt_count = 0;  // 每个网络尝试连接10次, 因此ac/10为此时连接的网络id(若该位置未存储网络则直接+10)
  void ApplyConfigMode();
  // 处理路由
  static void HandleIndex();
  static void HandleConnect();
  static void HandleNetworks();
  static void HandleDelete();
  static void HandleAiconfig();

  void SaveNetwork(String ssid, String password);
  void UpdateAllNetworksInPreferences();

public:
  // ai信息
  String llm_model;
  String llm_url;
  String llm_key;
  String img_key;
  
  Network();
  void Init();

  bool GetInitState();
  NETWORK_STATE GetConnectState();

  void SetConnectMode();
  void SetConfigMode();

  void ConnectSaved();

  void Loop();
};

#endif