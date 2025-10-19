import requests, json
# 仅为示例，请根据实际情况修改

url = "https://15252f15adb1fca493b9b25d11c4468f.apig.cn-north-4.huaweicloudapis.com/image_generation"

request = requests.post(url, json = {
    "llm_model": "deepseek-v3-250324",
    "llm_url": "https://ark.cn-beijing.volces.com/api/v3/chat/completions",
    "llm_key": "Bearer 2f24fa5f-4e5a-a002-394e-d22b402083a6",
    "img_key": "Bearer sk-imjbodjesktbejscafcnwleefjrskzghcrhjkawrpdqdlbah"
})
print(request.text)