#include "Network.h"

Network* Network::instance = nullptr;

// 配网界面
const char* configPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>配网</title>
    <meta charset="UTF-8">
    <style>
        body { font-family: Arial; margin: 40px; }
        h1 { color: #2c3e50; }
        form { background: #ecf0f1; padding: 20px; border-radius: 10px; }
        input { padding: 10px; margin: 10px 0; width: 100%; box-sizing: border-box; }
        button { background: #3498db; color: white; padding: 10px 15px; border: none; border-radius: 5px; cursor: pointer; }
        button:hover { background: #2980b9; }
        .saved-networks { margin-top: 30px; }
        .network-item { background: #f9f9f9; padding: 10px; margin: 5px 0; border-radius: 5px; }
    </style>
</head>
<body>
    <h1>WiFi配置</h1>
    <form action="/connect" method="POST">
        <h2>添加新网络</h2>
        <input type="text" name="ssid" placeholder="WiFi名称" required>
        <input type="password" name="password" placeholder="WiFi密码" required>
        <button type="submit">连接并保存</button>
    </form>

    <h1>AI配置</h1>
    <form action="/aiconfig" method="POST">
        <h2>语言模型</h2>
        <input type="text" name="llm_model" placeholder="模型名称" required>
        <input type="text" name="llm_url" placeholder="API URL" required>
        <input type="text" name="llm_key" placeholder="API Key" required>
        <h2>图像生成模型</h2>
        <input type="text" name="img_key" placeholder="API Key" required>
        <button type="submit">设置并保存</button>
    </form>

    <div class="saved-networks">
        <h2>已保存的网络</h2>
        <div id="savedList">
            <!-- 已保存的网络将在这里显示 -->
        </div>
    </div>
    
    <script>
        // 页面加载时获取已保存的网络
        window.onload = function() {
            fetch('/networks')
                .then(response => response.json())
                .then(data => {
                    const list = document.getElementById('savedList');
                    list.innerHTML = '';
                    
                    if (data.length === 0) {
                        list.innerHTML = '<p>暂无保存的网络</p>';
                        return;
                    }
                    
                    data.forEach((network, index) => {
                        const div = document.createElement('div');
                        div.className = 'network-item';
                        div.innerHTML = `
                            <strong>${network.ssid}</strong>
                            <button onclick="deleteNetwork(${index})">删除</button>
                            <button onclick="connectTo(${index})">连接</button>
                        `;
                        list.appendChild(div);
                    });
                });
        };
        
        function deleteNetwork(index) {
            fetch('/delete?index=' + index, { method: 'POST' })
                .then(() => location.reload());
        }
        
        function connectTo(index) {
            fetch('/connect?saved=' + index, { method: 'POST' })
                .then(() => {
                    alert('Attempting to connect...');
                });
        }
    </script>

</body>
</html>
)rawliteral";

Network::Network()
  : WebServer(3000) {
  if (!instance) instance = this;
};

void Network::Init() {
  // 使用单一的命名空间来存储所有网络信息
  prefs.begin("wifi_config", false);

  // 读取所有保存的网络
  for (int i = 0; i < MAX_NETWORK; i++) {
    String key_ssid = "ssid_" + String(i);
    String key_password = "password_" + String(i);
    ssid[i] = prefs.getString(key_ssid.c_str(), "");
    password[i] = prefs.getString(key_password.c_str(), "");
  }

  // 读取ai信息
  llm_model = prefs.getString("llm_model", "");
  llm_url = prefs.getString("llm_url", "");
  llm_key = prefs.getString("llm_key", "");
  img_key = prefs.getString("img_key", "");
  Serial.printf("llm_model: %s\n", llm_model);
}

NETWORK_STATE Network::GetConnectState() {
    return connectState;
}


void Network::SetConnectMode(){
  WiFi.mode(WIFI_STA);
  connect_attempt_time = millis();
  connectState = NETWORK_STATE_CONNECT_ATTEMPTING;
}

void Network::SetConfigMode() {
  connectState = NETWORK_STATE_CONFIG_PREPARE;
}

void Network::ApplyConfigMode() {
  WiFi.mode(WIFI_AP);
  connect_attempt_time = millis();
  connectState = NETWORK_STATE_CONFIG_PREPARE;

  IPAddress local_IP(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.softAPConfig(local_IP, gateway, subnet);
  WiFi.softAP("ArtisansTomeConfig", NULL);

  Serial.printf("配网AP已启动，SSID: ArtisansTomeConfig\n");
  Serial.printf("AP IP地址: %s\n", WiFi.softAPIP().toString().c_str());

  this->on("/", HTTP_GET, HandleIndex);
  this->on("/connect", HTTP_POST, HandleConnect);
  this->on("/networks", HTTP_GET, HandleNetworks);
  this->on("/delete", HTTP_POST, HandleDelete);
  this->on("/aiconfig", HTTP_POST, HandleAiconfig);

  this->begin();
  connect_attempt_time = millis();
  Serial.println("HTTP服务器已启动");
  connectState = NETWORK_STATE_CONFIG;
}

void Network::ConnectSaved() {
  // 检查是否已经连接成功
  if (WiFi.status() == WL_CONNECTED) {
    connectState = NETWORK_STATE_CONNECTED;
    Serial.printf("成功连接到: %s\n", WiFi.SSID().c_str());
    Serial.printf("IP地址: %s\n", WiFi.localIP().toString().c_str());
    return;
  }

  // 每500ms尝试一次连接
  if (millis() - connect_attempt_time > 500) {
    connect_attempt_time = millis();
    
    // 获取当前尝试的网络索引
    int network_index = attempt_count / 10;
    
    // 检查当前网络是否有效
    if (network_index >= MAX_NETWORK) {
      // 已经尝试完所有网络，从头开始
      attempt_count = 0;
      network_index = 0;
      Serial.println("已完成一轮网络尝试，重新开始...");
    }
    
    if (ssid[network_index] == "") {
      // 当前网络位置为空，跳过10次尝试
      Serial.printf("跳过空网络位置[%d]，直接跳到下一个\n", network_index);
      attempt_count = (network_index + 1) * 10; // 直接跳到下一个网络的开始
      return;
    }
    
    // 计算当前网络内的尝试次数(0-9)
    int attempt_in_network = attempt_count % 10;
    
    if (attempt_in_network == 0) {
      // 开始尝试新网络
      Serial.printf("开始尝试网络[%d]: %s\n", network_index, ssid[network_index].c_str());
      WiFi.begin(ssid[network_index].c_str(), password[network_index].c_str());
    }
    
    // 显示连接进度
    Serial.printf("尝试网络[%d] %s, 第%d次尝试\n", 
                 network_index, ssid[network_index].c_str(), attempt_in_network + 1);
    
    // 检查连接状态
    wl_status_t status = WiFi.status();
    if (status == WL_CONNECTED) {
      connectState = NETWORK_STATE_CONNECTED;
      Serial.printf("成功连接到: %s\n", ssid[network_index].c_str());
      Serial.printf("IP地址: %s\n", WiFi.localIP().toString().c_str());
      return;
    } else if (status == WL_CONNECT_FAILED || status == WL_NO_SSID_AVAIL) {
      // // 连接失败，直接跳到下一个网络
      // Serial.printf("网络[%d] %s 连接失败，跳到下一个\n", 
      //              network_index, ssid[network_index].c_str());
      // attempt_count = (network_index + 1) * 10; // 直接跳到下一个网络的开始
      // return;
    }
    
    // 移动到下一次尝试
    attempt_count++;
    
    // 如果已经尝试完所有网络且都没有连接成功
    if (attempt_count >= MAX_NETWORK * 10) {
      bool has_networks = false;
      for (int i = 0; i < MAX_NETWORK; i++) {
        if (ssid[i] != "") {
          has_networks = true;
          break;
        }
      }
      if (!has_networks) {
        Serial.println("没有保存的网络，切换到配置模式");
        SetConfigMode();
      } else {
        // 重置尝试计数，继续循环
        attempt_count = 0;
        Serial.println("所有网络尝试失败，重新开始循环...");
      }
    }
  }
  
  // 显示当前连接状态（每秒一次）
  static unsigned long last_status_time = 0;
  if (millis() - last_status_time > 1000) {
    last_status_time = millis();
    
    wl_status_t status = WiFi.status();
    const char* status_str = "";
    
    switch (status) {
      case WL_IDLE_STATUS: status_str = "空闲"; break;
      case WL_NO_SSID_AVAIL: status_str = "网络不可用"; break;
      case WL_SCAN_COMPLETED: status_str = "扫描完成"; break;
      case WL_CONNECTED: status_str = "已连接"; break;
      case WL_CONNECT_FAILED: status_str = "连接失败"; break;
      case WL_CONNECTION_LOST: status_str = "连接丢失"; break;
      case WL_DISCONNECTED: status_str = "未连接"; break;
      default: status_str = "未知状态";
    }
    
    Serial.printf("WiFi状态: %s\n", status_str);
  }
}

void Network::HandleIndex() {
  instance->send(200, "text/html", configPage);
}

void Network::HandleConnect() {
  String indexStr = instance->arg("index");
  String server_ssid;
  String server_password;

  if (indexStr != "") {
    server_ssid = instance->ssid[indexStr.toInt()];
    server_password = instance->password[indexStr.toInt()];
  } else {
    server_ssid = instance->arg("ssid");
    server_password = instance->arg("password");
  }

  if (server_ssid == "") {
    instance->send(400, "text/plain", "SSID不能为空");
    return;
  }

  Serial.printf("尝试连接网络: %s\n", server_ssid.c_str());

  // 尝试连接
  WiFi.begin(server_ssid.c_str(), server_password.c_str());
  for (int t = 0; t < 20; t++) {
    if (WiFi.status() == WL_CONNECTED) {
      instance->connectState = NETWORK_STATE_CONNECTED;
      Serial.printf("成功连接到: %s\n", server_ssid.c_str());

      // 保存网络到首选项
      instance->SaveNetwork(server_ssid, server_password);

      instance->send(200, "text/plain", "连接成功！设备将重启");
      delay(1000);
      ESP.restart();
      return;
    }
    delay(500);
    Serial.print(".");
  }

  instance->send(400, "text/plain", "连接失败，请检查密码或信号强度");
}

void Network::HandleAiconfig() {
  String new_llm_model = instance->arg("llm_model");
  String new_llm_url = instance->arg("llm_url");
  String new_llm_key = instance->arg("llm_key");
  String new_img_key = instance->arg("img_key");
  
  // 参数验证
  if (new_llm_model == "" || new_llm_url == "" || new_llm_key == "" || new_img_key == "") {
    instance->send(400, "text/plain", "错误：所有字段都必须填写");
    return;
  }
  
  // 保存到Preferences
  instance->prefs.putString("llm_model", new_llm_model);
  instance->prefs.putString("llm_url", new_llm_url);
  instance->prefs.putString("llm_key", new_llm_key);
  instance->prefs.putString("img_key", new_img_key);
  
  // 更新实例变量
  instance->llm_model = new_llm_model;
  instance->llm_url = new_llm_url;
  instance->llm_key = new_llm_key;
  instance->img_key = new_img_key;
  
  Serial.printf("AI配置已更新 - 模型: %s, URL: %s\n", 
                new_llm_model.c_str(), new_llm_url.c_str());
  
  // 发送成功响应
  instance->send(200, "text/html", 
    "<html><body>"
    "<h1>AI配置保存成功！</h1>"
    "<p><a href='/'>返回配置页面</a></p>"
    "</body></html>");
}


void Network::SaveNetwork(String ssid, String password) {
  // 检查是否已存在该网络
  for (int i = 0; i < MAX_NETWORK; i++) {
    if (instance->ssid[i] == ssid) {
      // 如果网络已存在，移动到最前面
      if (i > 0) {
        // 保存当前网络的SSID和密码
        String temp_ssid = instance->ssid[i];
        String temp_password = instance->password[i];

        // 将前面的网络向后移动一位
        for (int j = i; j > 0; j--) {
          instance->ssid[j] = instance->ssid[j - 1];
          instance->password[j] = instance->password[j - 1];
        }

        // 将当前网络放到最前面
        instance->ssid[0] = temp_ssid;
        instance->password[0] = temp_password;

        // 更新Preferences
        UpdateAllNetworksInPreferences();
      }
      return;
    }
  }

  // 如果不存在，将所有网络向后移动一位（队列操作）
  for (int i = 9; i > 0; i--) {
    instance->ssid[i] = instance->ssid[i - 1];
    instance->password[i] = instance->password[i - 1];
  }

  // 将新网络放在最前面
  instance->ssid[0] = ssid;
  instance->password[0] = password;

  // 更新Preferences
  UpdateAllNetworksInPreferences();
}

void Network::UpdateAllNetworksInPreferences() {
  // 将所有网络信息保存到Preferences
  for (int i = 0; i < MAX_NETWORK; i++) {
    String key_ssid = "ssid_" + String(i);
    String key_password = "password_" + String(i);

    if (instance->ssid[i] != "") {
      instance->prefs.putString(key_ssid.c_str(), instance->ssid[i]);
      instance->prefs.putString(key_password.c_str(), instance->password[i]);
    } else {
      // 如果为空，清除该位置的数据
      instance->prefs.remove(key_ssid.c_str());
      instance->prefs.remove(key_password.c_str());
    }
  }
}

void Network::HandleNetworks() {
  String json = "[";
  bool first = true;

  for (int i = 0; i < MAX_NETWORK; i++) {
    if (instance->ssid[i] != "") {
      if (!first) {
        json += ",";
      }
      json += "{\"ssid\":\"" + instance->ssid[i] + "\",\"index\":" + String(i) + "}";
      first = false;
    }
  }

  json += "]";
  instance->send(200, "application/json", json);
}

void Network::HandleDelete() {
  String indexStr = instance->arg("index");
  int index = indexStr.toInt();

  if (index >= 0 && index < MAX_NETWORK) {
    // 清除该位置的网络信息
    String key_ssid = "ssid_" + String(index);
    String key_password = "password_" + String(index);
    instance->prefs.remove(key_ssid.c_str());
    instance->prefs.remove(key_password.c_str());

    instance->ssid[index] = "";
    instance->password[index] = "";

    instance->send(200, "text/plain", "删除成功");
  } else {
    instance->send(400, "text/plain", "无效的索引");
  }
}

void Network::Loop() {
  if(millis() - connect_attempt_time > 300000){  // 5min未连接成功进入休眠
    connectState = NETWORK_STATE_TIMEOUT;
  }
  switch (connectState) {
    case NETWORK_STATE_CONNECTED:
      // 若运行过程中网络中断, 则试图重新连接
      if (WiFi.status() != WL_CONNECTED) {
        SetConnectMode();
      }
      break;
    case NETWORK_STATE_CONNECT_ATTEMPTING:
      ConnectSaved();
      break;
    case NETWORK_STATE_CONFIG:
      // 保持配网模式的服务器运行
      handleClient();
      if (WiFi.getMode() != WIFI_AP && WiFi.getMode() != WIFI_AP_STA) {
        Serial.println("AP意外关闭，重新启动AP...");
        SetConfigMode();
      }
      break;
    case NETWORK_STATE_CONFIG_PREPARE:  // 防止在中断中执行ApplyConfigMode在300ms以上导致看门狗错误
      ApplyConfigMode();
      break;
  }
}
