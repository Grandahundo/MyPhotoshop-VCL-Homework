import os
import random
import numpy as np
from PIL import Image, ImageDraw, ImageFilter

# 配置信息
OUTPUT_DIR = "assets"
if not os.path.exists(OUTPUT_DIR):
    os.makedirs(OUTPUT_DIR)

def save_brush(image, name):
    path = os.path.join(OUTPUT_DIR, f"brush_{name}.png")
    image.save(path)
    print(f"成功生成笔刷: {path}")

def get_circular_mask(size, feather=0.9):
    """生成径向渐变遮罩，控制边缘硬度"""
    mask = np.zeros((size, size), dtype=np.float32)
    center = size / 2
    y, x = np.ogrid[:size, :size]
    dist = np.sqrt((x - center)**2 + (y - center)**2)
    
    # 在半径内进行线性衰减
    mask = np.clip((center - dist) / (center * (1 - feather + 1e-6)), 0, 1)
    return mask

def make_rgba_white(size, alpha_array):
    """根据 Alpha 阵列生成纯白 RGBA 图像"""
    img = Image.new("RGBA", (size, size), (255, 255, 255, 0))
    alpha_img = Image.fromarray(alpha_array.astype(np.uint8), mode='L')
    img.putalpha(alpha_img)
    return img

# --- 1. 基础/真实类笔刷 ---

def gen_crayon(size=256):
    mask = get_circular_mask(size, 0.8)
    noise = np.random.randint(120, 255, (size, size)).astype(np.float32)
    # 随机空洞模拟积蜡感
    holes = (np.random.rand(size, size) > 0.15)
    alpha = np.clip(noise * mask * holes, 0, 255)
    save_brush(make_rgba_white(size, alpha), "crayon")

def gen_pencil(size=128):
    mask = get_circular_mask(size, 0.4)
    # 极细碎的颗粒
    noise = np.random.normal(180, 60, (size, size)).astype(np.float32)
    alpha = np.clip(noise * mask, 0, 255)
    save_brush(make_rgba_white(size, alpha), "pencil")

def gen_watercolor(size=512):
    mask = get_circular_mask(size, 0.1) # 极软边缘
    # 模拟水渍堆积边缘效果
    wet_edge = get_circular_mask(size, 0.98) - get_circular_mask(size, 0.85)
    alpha = (mask * 100) + (wet_edge * 120)
    img = make_rgba_white(size, np.clip(alpha, 0, 255))
    img = img.filter(ImageFilter.GaussianBlur(radius=8))
    save_brush(img, "watercolor")

# --- 2. 艺术/质感类笔刷 ---

def gen_oil(size=256):
    mask = get_circular_mask(size, 0.9)
    alpha = np.zeros((size, size), dtype=np.float32)
    for i in range(0, size, 2): # 纵向纹路
        if random.random() > 0.2:
            alpha[:, i:i+1] = random.randint(180, 255)
    img = make_rgba_white(size, alpha * mask)
    img = img.rotate(45).filter(ImageFilter.GaussianBlur(radius=1))
    save_brush(img, "oil")

def gen_chalk(size=256):
    mask = get_circular_mask(size, 0.7)
    # 离散的多孔纹理
    noise = np.random.choice([0, 150, 255], size=(size, size), p=[0.7, 0.1, 0.2]).astype(np.float32)
    img = make_rgba_white(size, noise * mask)
    img = img.filter(ImageFilter.MedianFilter(size=3))
    save_brush(img, "chalk")

def gen_ink(size=256):
    mask = get_circular_mask(size, 0.95)
    alpha = (get_circular_mask(size, 0.7) * 255).astype(np.float32)
    img = make_rgba_white(size, alpha * mask)
    img = img.filter(ImageFilter.GaussianBlur(radius=3))
    save_brush(img, "ink")

# --- 3. 夸张/特效类笔刷 ---

def gen_star(size=256):
    img = Image.new("RGBA", (size, size), (255, 255, 255, 0))
    draw = ImageDraw.Draw(img)
    center = size // 2
    pts = []
    for i in range(10):
        angle = i * np.pi / 5 - np.pi / 2
        r = (size * 0.45) if i % 2 == 0 else (size * 0.18)
        pts.append((center + r * np.cos(angle), center + r * np.sin(angle)))
    draw.polygon(pts, fill=(255, 255, 255, 255))
    img = img.filter(ImageFilter.GaussianBlur(radius=1))
    save_brush(img, "star")

def gen_smoke(size=512):
    mask = get_circular_mask(size, 0.0)
    alpha = np.zeros((size, size), dtype=np.float32)
    for _ in range(20):
        x, y = random.randint(size//4, 3*size//4), random.randint(size//4, 3*size//4)
        r = random.randint(size//8, size//4)
        # 叠加随机的高斯团
        y_grid, x_grid = np.ogrid[:size, :size]
        d = np.sqrt((x_grid - x)**2 + (y_grid - y)**2)
        alpha += np.clip(255 * (1 - d/r), 0, 255) * 0.3
    alpha = np.clip(alpha * mask, 0, 255)
    img = make_rgba_white(size, alpha)
    img = img.filter(ImageFilter.GaussianBlur(radius=12))
    save_brush(img, "smoke")

def gen_glitch(size=256):
    mask = get_circular_mask(size, 0.9)
    alpha = np.zeros((size, size), dtype=np.uint8)
    img_alpha = Image.fromarray(alpha, mode='L')
    draw = ImageDraw.Draw(img_alpha)
    for _ in range(35):
        x, y = random.randint(0, size), random.randint(0, size)
        w, h = random.randint(15, 70), random.randint(3, 12)
        draw.rectangle([x, y, x+w, y+h], fill=random.randint(150, 255))
    final_alpha = np.array(img_alpha).astype(np.float32) * mask
    save_brush(make_rgba_white(size, final_alpha), "glitch")

def gen_grass(size=256):
    img = Image.new("RGBA", (size, size), (255, 255, 255, 0))
    draw = ImageDraw.Draw(img)
    center = size // 2
    for _ in range(80):
        angle = random.uniform(0, 2 * np.pi)
        length = random.uniform(size*0.1, size*0.48)
        end_x = center + length * np.cos(angle)
        end_y = center + length * np.sin(angle)
        draw.line([center, center, end_x, end_y], fill=(255,255,255,random.randint(180, 255)), width=2)
    save_brush(img, "grass")

def gen_neon(size=256):
    mask = get_circular_mask(size, 0.2)
    alpha = np.zeros((size, size), dtype=np.float32)
    # 多层辉光
    for r_factor, weight in [(0.48, 40), (0.3, 80), (0.15, 200)]:
        m = get_circular_mask(size, 1.0 - r_factor)
        alpha = np.maximum(alpha, m * weight)
    img = make_rgba_white(size, alpha)
    img = img.filter(ImageFilter.GaussianBlur(radius=4))
    save_brush(img, "neon")

def gen_spray(size=512):
    mask = get_circular_mask(size, 0.0)
    noise = np.random.rand(size, size).astype(np.float32)
    # 基于距离的概率散射
    alpha = (noise < (mask**3)) * 255 * mask
    img = make_rgba_white(size, alpha)
    img = img.filter(ImageFilter.GaussianBlur(radius=0.8))
    save_brush(img, "spray")

# --- 主程序 ---

if __name__ == "__main__":
    print("开始生成夸张笔刷纹理...")
    
    gen_crayon()
    gen_pencil()
    gen_watercolor()
    gen_oil()
    gen_chalk()
    gen_ink()
    gen_star()
    gen_smoke()
    gen_glitch()
    gen_grass()
    gen_neon()
    gen_spray()
    
    print("\n所有笔刷已生成至 assets 目录下！")
    print("现在启动你的 C++ 程序，它们应该会被自动加载。")