import socketserver
import http.server
from urllib.parse import parse_qs, urlparse
import json
import requests
import numpy as np
import io
import os
from PIL import Image, ImageDraw, ImageFont
import ref
import hmac
import hashlib
import hmac
import hashlib
import base64
import email.utils
import tempfile

PORT = 8000
params = {}

def oss_upload(path, content):
    try:
        bucket_name = 'newbucket-yourname'
        endpoint = 'oss-cn-shenzhen.aliyuncs.com'
        method = 'PUT'
        content_type = 'application/octet-stream'
        date_str = email.utils.formatdate(usegmt=True)
        access_key_id = str(os.getenv('OSS_ACCESS_KEY_ID'))
        access_key_key = str(os.getenv('OSS_ACCESS_KEY_SECRET'))
        canonicalized_oss_headers = ''

        canonicalized_resource = f'/{bucket_name}/{path}'
        string_to_sign = f"{method}\n\n{content_type}\n{date_str}\n{canonicalized_oss_headers}{canonicalized_resource}"
        h = hmac.new(
            access_key_key.encode('utf-8'), 
            string_to_sign.encode('utf-8'), 
            hashlib.sha1
        )
        signature = base64.b64encode(h.digest()).strip().decode('utf-8')
        
        url = f'https://{bucket_name}.{endpoint}/{path}'
        headers = {
            'Authorization': f'OSS {access_key_id}:{signature}',
            'Date': date_str,
            'Content-Type': content_type
        }

        response = requests.put(url, data=content, headers=headers)
        if response.status_code != 200:
            raise ValueError(f"err in oss_upload: {response.text}")
        
        return response
    except Exception as err:
        import sys
        raise ValueError("err in img_request: line" + str(sys.exc_info()[2].tb_lineno) + str(err))

def loadfont(base64_str, size: int=18):
    """使用临时文件方法加载ref中的base64字体,绕过所有编码问题"""
    
    try:
        # 清理和解码
        clean_b64 = base64_str.replace('\n', '').replace(' ', '')
        font_data = base64.b64decode(clean_b64)
        
        # 创建临时文件
        with tempfile.NamedTemporaryFile(delete=False, suffix='.ttf') as temp_file:
            temp_file.write(font_data)
            temp_path = temp_file.name
        
        try:
            # 从临时文件加载字体
            font = ImageFont.truetype(temp_path, size=size)
            return font
        finally:
            # 清理临时文件
            try:
                os.unlink(temp_path)
            except:
                pass
                
    except Exception as e:
        return None   

def get_llm_content() -> str:
    # 随机生成内容
    age = np.random.randint(12, 70)
    role = np.random.choice(["农业/种植类", "工匠/工业/技术类", "养殖/畜牧类", "公共事务/法律类", "军事/安保类", 
            "销售/贸易类", "文化/教育类", "宗教/术士/哲学类", "医疗/护理/卫生类", "表演/娱乐类", 
            "交通/运输/物流类", "采集/狩猎/渔业类", "能源/矿产类", "服务业类", "新闻/媒体/出版类",
            "科技/研发类", "手工业/纺织类", "建筑/园林类", "食品/餐饮类", "天文/地理类", 
            "摊贩/店铺类", "金融/经济/保险类", "艺术/创作类", "考古/历史类", "生老病死/婚姻家庭类",
            "体育/运动类"
            ])
    if(np.random.random() < 0.85):
        era_selection = ["西周", "秦代", "汉代", "三国", 
                         "唐代", "南宋", "明代", "清代", 
                         "民国", "1960年", "1980年", "2000年", 
                         "2020年", "2035年(注意不能离现实过远)"]
        style_selection = [
            "铭文风格，参考先秦时期器物上的铭文，句式短促，以四言为主；简约古拙，多使用“唯”、“其”等古语，庄重肃穆。",
            "诗经风格，严格四言诗，必须使用“兴”的手法（先言他物以引起所咏之词）；押古韵；词汇质朴，多使用重章叠句。质朴自然，多提及自然景物，生活气息浓厚。",
            "律令风格，如秦代法律条文或官方诏书（文言文），严谨、精确、冷峻，排除任何比喻和感情色彩，体现强制性，充满权威感和秩序感。",
            "耕战风格，使用秦代文言文，务实、粗犷，内容紧扣耕作、军事等，反映法家统治下庶民与士兵的生活，用语直接有力",
            "辞赋风格，大量运用“其X则...，其Y则...”的铺陈结构；必须使用对仗；词汇华丽，堆砌典故与奇字，善用排比与典故，词汇丰富绚烂，充满浪漫与想象。",
            "乐府风格，五言古诗体；叙事性强，常有乐府套语；语言清新流畅，带有民间口语色彩，反映社会现实，情感真挚动人",
            "演义风格，带有浪漫化、戏剧化的英雄史诗色彩，文言文，善用典故与比喻，语言气势磅礴，强调忠义、谋略与个人武勇，有鲜明的立场与情感倾向。",
            "史书风格，参考《三国志》行文，使用文言文，语言严谨、客观，反映历史事件和人物，带有一定的历史观和道德观，但不堆砌词藻。",
            "诗豪风格，五言或七言律诗（8句）格式，如李白、李贺的诗歌，自信奔放，善用夸张想象，充满奇特的想象和磅礴气势。",
            "禅意风格，五言或七言律诗（8句）格式，类似王维、寒山的诗句，空灵含蓄，言简意赅，追求意境深远，带有佛道哲学的韵味。",
            "婉约风格，严格遵循《青玉案》上阙词牌格律：五句，三十三字。句式结构为七，三，三。七。四，四，五。第1,3,4,6,7句押仄声韵，一韵到底。语言风格婉约精致，意象密集细腻，善于描绘内心细腻的情感与景物。",
            "豪放风格，必须严格遵守《江城子》上阕格律：7句，33字，4平韵（韵脚在第三、五、六、七句尾字）。句式结构为：7字, 3字, 3字, 9字, 7字, 3字, 3字。语言豪放沉雄，融入边塞、江山等壮阔意象，善于描绘壮丽景象，格局开阔，显家国情怀。",
            "话本风格，如同明代章回小说，口语化、故事性强，生动有趣，贴近市井，充满商业与市井生活气息。",
            "笔记风格，明代通俗文学，语言趋于平实、细腻、生活化，常带有品鉴、把玩的意味，如同文人笔记中的记录。",
            "醒世风格，文白夹杂，强调忧患意识、批判性与变革图强的急迫感，新旧思想在词句中碰撞，力图启蒙。",
            "训诂风格，考据严谨，引经据典，追求溯源与精准。语言倾向于学术化，带有一种整理国故、集大成的考究与繁复感。",
            "白话文风格，采用早期白话文特征：保留“的、地、得”分化不清的用法；词汇上文白交替，兼具古典韵味与现代启蒙的简洁感，带有革新、探索与忧患意识的色彩。",
            "申报风格，如同民国老报纸的新闻体或广告语采用“新闻导语体”：首句概括核心事件；正文遵循“5W1H”原则；语言客观，但标题可稍显夸张。",
            "口号风格，语言类似新中国建设的口号，采用具有动员力的句式，包含“人民”、“革命”、“建设”、“万岁”等核心词汇。简洁有力，充满集体主义、革命理想主义和劳动热情。",
            "日记风格，朴实、真诚，带有个人崇拜和无私奉献的情感色彩，如《雷锋日记》般亲切感人。",
            "新风风格，充满希望、憧憬与反思，语言开始变得个性化、抒情化，带有改革开放初期的理想主义。",
            "市井风格，鲜活、接地气，反映市井生活的复苏，带有烟火气和商业萌芽感。",
            "网络初兴风格，互联网兴起，语言趋向多元化、网络化、商业化，常带有广告语、流行文化的影子与千禧年的乐观与开放。",
            "全球化风格，新世纪的时尚、多元，词汇中融入英文缩写和新兴概念，节奏明快。",
            "自媒体风格，自媒体时代语言，碎片化、吸睛，善用梗、表情包化和夸张语气，追求即时传播和情绪共鸣。",
            "极简风格，冷静、精确，如同产品说明书或科技评测，注重效率、数据和用户体验，不要列具体数据。",
            "智能纪实风格，常用“AI 协同”“能源循环”“生态联动” 等术语，兼顾科技感与生活气息，语言简洁直白，侧重物品与日常智能场景的关联",
            "未来叙事风格，聚焦“双碳目标”下的绿色生活，语言带温和的生态关怀，描述中融入自然意象（如“光伏叶”“潮汐能”），兼具纪实与诗意"
        ]
        era_index = np.random.randint(0, len(era_selection))
        era = era_selection[era_index]
        style = style_selection[era_index * 2 + np.random.randint(0, 2)]
    else:
        era_selection = ["史前", "2277年", "魔法时代(科技水平相当于宋代)", "水底世界", "末世", "蒸汽朋克大明", 
                         "东方志怪(现代都市与东方妖鬼精怪共存的世界)", 
                         "万物有灵(所有物体都拥有情感与记忆,偏向自然而非人类活动)"]
        style_selection = [
            "神话风格，采用“创世史诗”的句式：多使用排比、复沓结构；词汇宏大，语言充满对自然力量的敬畏，将万物拟人化、神化，描述如创世史诗般宏大而神秘。",
            "岩画风格，句式短促，仅由“名词+动词”或“名词+形容词”构成（如“太阳。燃烧。”），极其简练、具象，如同刻在岩壁上的符号，禁止使用比喻和抽象词汇；只描述可见的动作与物体。",
            "AI逻辑风格，极度理性、精准，不带感情色彩，语句结构可能非常规，带有算法分析和数据报告的冰冷感。",
            "意识流风格，跳跃、非线性，围绕‘记忆-数据-梦境’三个核心概念进行跳跃性联想，反映高度互联或混乱的意识状态。",
            "咒语风格，韵律感强，类似魔法咒语，采用“三段式”结构（呼唤-描述-命令），多使用隐喻和象征，晦涩难懂但充满神秘的美感。",
            "学徒笔记风格，使用现代文，以公历日期“【x月x日】”开头；内容为“现象-猜想-尝试-结果”的逻辑链，充满好奇、尝试性，夹杂着成功与失败的记录，像是魔法学徒的实践手札，生动而琐碎。",
            "潮汐风格，语言流动、空灵，节奏如波浪般起伏，多使用与水流、光影、声音相关的柔和词汇，避免任何尖锐、急促的词汇。",
            "航海日志风格，“第N个潮汐周期”开头，具有客观、记录性，但细节中透露出对未知深渊的警惕与好奇，如潜水员的观察笔记。",
            "启示录风格，破碎、压抑，常有不合逻辑的断裂和重复，仿佛在回忆创伤时思绪的中断。",
            "日志风格，务实、警惕，语言直接且充满生存智慧，专注于记录末世生存细节，备注栏用最简练的语言说明用途或风险（如“-可交易。-小心哑火”）；完全剔除形容词和感情描写。",
            "机巧辞赋风格，以古典赋文的华丽辞藻描绘机械造物，大量运用四六骈文，充满东方古典与工业奇想的融合之美。",
            "匠册风格，如同古代工程手册的笔录，用语精准、务实，夹杂着使用工匠术语、计量单位和实测数据，朴实中见巧思。",
            "志怪风格，现代文，笔调冷峻奇诡，如同《聊斋》或《子不语》。在描述现代物品时赋予其妖异、神秘的特性，营造出抽离现实的诡异感。",
            "都市传说风格，以现代都市人的口吻讲述不可思议的见闻。语言贴近生活却暗藏玄机，如同朋友间分享的诡异经历，真实与虚构的界限模糊。",
            "灵语风格，古老、诗意而缓慢。语言充满通感，句子都混合两种以上的感官，物品的叙述视角宏大，与自然和时空紧密相连。",
            "物语风格，以物品本身作为第一人称（解除不使用第一人称限制），语言天真、好奇或带着岁月的絮叨，充满主观感受和细微的感知，包含物品的“小情绪”或“小偏好”。",
        ]
        era_index = np.random.randint(0, len(era_selection))
        era = era_selection[era_index]
        style = style_selection[era_index * 2 + np.random.randint(0, 2)]

    if(np.random.random() > 0.6):
        culture = np.random.choice(["江南", "岭南", "巴蜀", "中原", "西北", "东北", "西域",
        "燕京", "滇南", "徽州", "荆楚", "齐鲁", "关中", "青藏", 
        "草原", "海滨", "海岛", "客家", "闽南", "吴越", "壮乡"])
    else:
        culture = ""

    if(np.random.random() > 0.6):
        festival = np.random.choice(["春节", "元宵", "端午", "中秋", "清明", "七夕", 
            "中元", "腊八", "庆典", "祈福", "收获", "重阳",
            "立春", "雨水", "惊蛰", "春分", "清明", "谷雨",
            "立夏", "小满", "芒种", "夏至", "小暑", "大暑",
            "立秋", "处暑", "白露", "秋分", "寒露", "霜降",
            "立冬", "小雪", "大雪", "冬至", "小寒", "大寒"])
    else:
        festival = ""

    price = int(10 ** (np.random.random() * 4))
    if(np.random.random() < 0.7):
        # typ = "生成一项与你的职业特征强相关的工具，原料，产品或物品"
        usage = np.random.choice(["你用于劳动的工具", "你制造的产品", "你用于生产的原料", "代表你职业的标志", "你工作时穿戴的装备"])
        typ = "生成一项" + usage
        print(usage[-2:])
    else:
        if(np.random.random() < 0.6):  # 概率随机化物品功能
            usage = np.random.choice([
                        "容器", "生活用品", "日常工具或仪器", "配饰",
                        "衣物/防护用品", "交通工具或用品", "休闲/娱乐用品", "收藏品",
                        "纸质物品", "动物/植物/矿物制品", "食品/药品", "声/光/电/温度物品"
                ])
        else:
            usage = "物品"

        print(usage)

        if(np.random.random() < 0.5):
            typ = "生成一项与你的职业弱相关或无关，但你可能会携带或使用的" + usage
        else:
            typ = "生成一项你从" + np.random.choice(["父母", "朋友", "邻居", (("子女" if (age > 44) else "配偶") if (age > 22) else "长辈")]) + "获得的" + usage

    gender = np.random.choice(["男", "女"])
    area = np.random.choice(["城市", "乡村", "野外"])

    print(role, gender, age, era.split("(")[0], area, price, culture, festival, style.split("风格，")[0])
    style = style.split("风格，")[1]

    return "你是一位来自" + era + "中国" + area + "，" + str(age) + "岁的" + gender + "性，从事" + role + "职业，" + \
    (("来自" + culture) if (culture != "") else "") + '''，请根据以下约束生成内容：
    role：你的具体职业（注意是名词，具体而简短，8字以内），该职业类型属于''' + role + '''
    item：''' + typ + (("，与" + festival + "相关") if (festival != "") else "") + '''（8字以内，不要带括号），
    该场景下参考物品品质：草帽5/酒30/铁锄50/米10/绢200/牛1500，该物品品质为''' + str(price) + '''

    description：一段描述性文字，涉及其特征、功能、来历、故事等，不使用第一人称
    - 长度：保证在60字符以上、75字符以下（计算标点）
    - 语言风格：''' + style + '''
    - 不要带有emoji

    prompt：用于文生图的提示词
    - 必须包含：物品材质+形态+颜色+细节特征，白色背景
    - 禁止出现：拼接碎片、透视变形
    - 需描述物品形态、材质、颜色、典型特征等，尽可能具象（如古朴纹样->螺旋状云纹/锯齿状回纹等）
    - 描述物品时语言需简洁而准确，不要带具体数据
    - 对于物品古称或带有比喻的名称，需换成便于文生图理解的描述（如耒->带齿的木铲）

    严格按照以下示例输出json：
    {
    "role":string
    "item": string
    "description": string
    "prompt"：string
    }
    '''

def llm_request() -> dict:
    # deepseek
    try:
        deepseek_request = requests.post(params.get('llm_url'), json={
            "model": params.get('llm_model'),
            "messages": [
                    {
                        "role": "user",
                        "content": get_llm_content()
                    }
                ]
            }, headers={
                "Content-Type": "application/json",
                "Authorization": params.get('llm_key')
            })

        deepseek_request_json = deepseek_request.json()
        # image_prompt = deepseek_request_json["choices"][0]["message"]["content"]
        response_content = deepseek_request_json["choices"][0]["message"]["content"]
        # 去掉可能的```json ```
        response_content = json.loads(response_content.replace("json", "").replace("```", ""))
        return response_content
    except Exception as err:
        import sys
        raise ValueError("err in llm_request: line" + str(sys.exc_info()[2].tb_lineno) + str(err))
    
def img_request(prompt: str) -> bytes:
    try:
        image_request = requests.post("https://api.siliconflow.cn/v1/images/generations", json={
            "model": "Kwai-Kolors/Kolors",
            "prompt": prompt,
            "image_size": "1024x1024",
            "batch_size": 1,
            "image": ref.ref_image
        }, headers={
            "Content-Type": "application/json",
            "Authorization": params.get('img_key')
        })

        image_request_json = image_request.json()
        image_download_url = image_request_json["images"][0]["url"]

        # 下载图片
        response = requests.get(image_download_url, verify=False)
        return response.content
    except Exception as err:
        import sys
        raise ValueError("err in img_request: line" + str(sys.exc_info()[2].tb_lineno) + str(err))

def img_process(img: Image, content: dict, font_large: ImageFont.FreeTypeFont, font_small: ImageFont.FreeTypeFont) -> Image:
    # 转换灰度并缩放
    img = img.convert('L')
    img = img.resize((300, 300))

    # 归一化像素值到[0, 1]范围
    # img = ImageOps.equalize(img)
    pixels = np.array(img, dtype=np.float32) / 255.0

    # 获取图像尺寸
    height, width = pixels.shape

    # 随机抖动处理
    for i in range(1, height - 1):
        for j in range(1, width - 1):
            if pixels[i, j] > np.random.random():
                pixels[i, j] = 1.0
            else:
                pixels[i, j] = 0.0

    # 将结果转换回[0, 255]范围并转换为PIL图像
    output_image = Image.fromarray((pixels * 255).astype(np.uint8))

    # 在图片下方添加宽度300, 高度100的白色区域
    white_region = Image.new('L', (300, 100), 255)
    final_image = Image.new('L', (300, 400))
    final_image.paste(output_image, (0, 0))
    final_image.paste(white_region, (0, 300))

    # 图片下方写字
    for i in range(content["item"].__len__()):
        draw = ImageDraw.Draw(final_image)
        draw.text((20 * i + 1, 301), content["item"][i], font=font_large)

    for i in range(content["role"].__len__()):
        draw = ImageDraw.Draw(final_image)
        draw.text((283 - i * 20, 301), content["role"][content["role"].__len__() - i - 1], font=font_large)

    for i in range(np.min([content["description"].__len__(), 100])):
        draw = ImageDraw.Draw(final_image)
        draw.text((15 * (i % 20) + 1, 324 + i // 20 * 15), content["description"][i], font=font_small)

    # 图像逆时针旋转90度
    final_image = final_image.transpose(Image.ROTATE_90)
    return final_image

def img_to_bytes(img: Image) -> bytes:
    # 转换为bytes, 每个byte储存8个像素
    image_bytes_black = bytearray()

    img = np.array(img)
    for i in range(img.shape[0]):
            for j in range((img.shape[1]) // 8):
                byte_black = 0
                for k in range(8):
                    if img[i, j * 8 + k] > 128:
                        byte_black |= 1 << (7 - k)
                    else:
                        pass
                image_bytes_black.append(byte_black)
    return image_bytes_black

class SimpleHTTPRequestHandler(http.server.SimpleHTTPRequestHandler):
    def do_POST(self):

        self.send_response(200)
        self.send_header('Content-type', 'text/html')
        self.end_headers()

        status = 0

        # 读取请求体内容
        content_length = int(self.headers['Content-Length'])
        post_data = self.rfile.read(content_length)
            
        # 解析JSON参数
        global params 
        params = json.loads(post_data.decode('utf-8'))
        
        try:
            response_content = llm_request()

            font_large = loadfont(ref.font, 18)
            if font_large is None:
                raise Exception("字体加载失败")
            font_small = loadfont(ref.font, 12)

            image_bytes = img_request(response_content["prompt"])

            # 使用PIL打开图像
            img = Image.open(io.BytesIO(image_bytes))
            final_image = img_process(img, response_content, font_large, font_small)

            # 转换为bytes, 每个byte储存8个像素
            image_bytes_black = img_to_bytes(final_image)

            # 存储到oss
            oss_upload("a.img", image_bytes_black)

        except Exception as err:
            import sys
            message = {
                "status": -1,
                "errline": sys.exc_info()[2].tb_lineno,
                "errinfo": str(err)
            }
            self.wfile.write(json.dumps(message).encode())
            return

        message = {
            "status": status,
        }
        self.wfile.write(json.dumps(message).encode())

with socketserver.TCPServer(('127.0.0.1', PORT), SimpleHTTPRequestHandler) as httpd:
    print('Python web server at port 8000 is running..')
    httpd.serve_forever()