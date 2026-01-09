import os
from PIL import Image, ImageDraw, ImageFilter
import numpy as np
import random

# 创建保存目录
output_dir = "assets"
if not os.path.exists(output_dir):
    os.makedirs(output_dir)

def save_brush(image, name):
    path = os.path.join(output_dir, f"brush_{name}.png")
    image.save(path)
    print(f"已生成笔刷: {path}")

def get_circular_mask(size, feather=0.9):
    """生成一个圆形的 Alpha 遮罩，确保边缘干净"""
    mask = np.zeros((size, size), dtype=np.float32)
    center = size / 2
    for y in range(size):
        for x in range(size):
            dist = np.sqrt((x - center)**2 + (y - center)**2)
            # 半径内的衰减
            if dist < center:
                # feather 控制边缘硬度，1.0 是硬圆，越小越模糊
                opacity = np.clip((center - dist) / (center * (1 - feather + 0.01)), 0, 1)
                mask[y, x] = opacity
    return mask

def create_base_white_image(size, alpha_data):
    """将 Alpha 数据应用到纯白图像上"""
    img = Image.new("RGBA", (size, size), (255, 255, 255, 0))
    alpha_channel = Image.fromarray(alpha_data.astype(np.uint8), mode='L')
    img.putalpha(alpha_channel)
    return img

def create_crayon(size=256):
    mask = get_circular_mask(size, 0.8)
    # 强烈的随机噪点
    noise = np.random.randint(100, 255, (size, size)).astype(np.float32)
    # 增加一点块状感
    alpha = noise * mask * (np.random.rand(size, size) > 0.2)
    save_brush(create_base_white_image(size, alpha), "crayon")

def create_pencil(size=128):
    mask = get_circular_mask(size, 0.5)
    noise = np.random.normal(200, 50, (size, size)).astype(np.float32)
    alpha = np.clip(noise * mask, 0, 255)
    save_brush(create_base_white_image(size, alpha), "pencil")

def create_watercolor(size=512):
    mask = get_circular_mask(size, 0.1) # 极软的边缘
    alpha = (mask * 150).astype(np.float32)
    # 模拟水渍边缘：强化边缘
    edge_enhancement = get_circular_mask(size, 0.95) - get_circular_mask(size, 0.85)
    alpha += edge_enhancement * 100
    # 模糊处理
    img = create_base_white_image(size, np.clip(alpha, 0, 255))
    img = img.filter(ImageFilter.GaussianBlur(radius=5))
    save_brush(img, "watercolor")

def create_oil(size=256):
    """油画：具有条纹纹理"""
    mask = get_circular_mask(size, 0.9)
    alpha = np.zeros((size, size), dtype=np.float32)
    for i in range(size):
        # 创建纵向条纹
        line_noise = np.random.randint(150, 255)
        if random.random() > 0.3:
            alpha[:, i] = line_noise
    # 模糊和旋转模拟刷毛
    img = create_base_white_image(size, alpha * mask)
    img = img.rotate(random.randint(0, 360))
    img = img.filter(ImageFilter.GaussianBlur(radius=1))
    save_brush(img, "oil")

def create_chalk(size=256):
    """粉笔：松散、多孔"""
    mask = get_circular_mask(size, 0.7)
    noise = np.random.choice([0, 200, 255], size=(size, size), p=[0.6, 0.2, 0.2]).astype(np.float32)
    img = create_base_white_image(size, noise * mask)
    img = img.filter(ImageFilter.MedianFilter(size=3))
    save_brush(img, "chalk")

def create_spray(size=512):
    """喷漆：颗粒状扩散"""
    mask = get_circular_mask(size, 0.0) # 极其发散
    # 距离中心的概率分布
    noise = np.random.rand(size, size).astype(np.float32)
    alpha = (noise < mask) * 255 * (mask ** 2)
    img = create_base_white_image(size, alpha)
    img = img.filter(ImageFilter.GaussianBlur(radius=1))
    save_brush(img, "spray")

def create_ink(size=256):
    """墨水：边缘轻微扩散"""
    mask = get_circular_mask(size, 0.9)
    img = Image.new("L", (size, size), 0)
    draw = ImageDraw.Draw(img)
    draw.ellipse([size*0.1, size*0.1, size*0.9, size*0.9], fill=255)
    # 扩散效果
    img = img.filter(ImageFilter.GaussianBlur(radius=10))
    alpha = np.array(img).astype(np.float32) * mask
    save_brush(create_base_white_image(size, alpha), "ink")

if __name__ == "__main__":
    create_crayon()
    create_pencil()
    create_watercolor()
    create_oil()
    create_chalk()
    create_spray()
    create_ink()