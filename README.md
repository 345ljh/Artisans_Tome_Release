# 百工谱——AIGC物品展示框
## Artisan's Tome

「一物一世界，一触一春秋」
一款基于ESP32的智能桌面艺术装置。
基于预设库中时代、地域、职业等元素与文化符号，通过图像生成模型，创造出一件独一无二的虚拟物品，在电子墨水屏上呈现独特的视觉体验，让每一次交互都成为在历史长河中的一次随机发现，让科技与人文在方寸之间完美交融。

/3d：3D模型，包括FreeCAD工程文件与stl模型。
/documentation：项目详细文档。
/pcb：立创EDA工程文件、BOM表、Gerber文件。
/screen：ESP32软件。
/handbook：使用手册。
/image_generation：
 - bootstrap：FunctionGraph项目启动文件。
 - hw.py：FunctionGraph上的主代码。
 - image_generation.py：本地图像生成代码。
 - ref.py：base64格式的参考图像。
 - test_client.py：FunctionGraph测试客户端。
 - zpix.ttf：像素字体文件。