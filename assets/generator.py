from PIL import Image, ImageDraw, ImageFilter
import numpy as np
import random

def save_brush(image, name):
    image.save(f"{name}.png")
    print(f"已生成笔刷: {name}.png")

def create_crayon(size=256):
    """
    蜡笔：边缘破碎，内部有强烈的随机噪点（积蜡感）
    """
    # 创建透明背景
    img = Image.new("RGBA", (size, size), (255, 255, 255, 0))
    canvas = np.zeros((size, size), dtype=np.uint8)
    
    center = size // 2
    radius = size // 2 - 10
    
    for y in range(size):
        for x in range(size):
            dist = np.sqrt((x - center)**2 + (y - center)**2)
            if dist < radius:
                # 越往边缘，消失概率越高（破碎感）
                edge_factor = (radius - dist) / radius
                # 混合多种噪点
                noise = random.randint(0, 255) 
                if noise > (100 + 155 * edge_factor):
                    canvas[y, x] = 0
                else:
                    # 随机透明度，模拟蜡笔留白
                    canvas[y, x] = random.randint(150, 255)
    
    # 转换为图像并添加轻微模糊，使其不过于锐利
    alpha_channel = Image.fromarray(canvas, mode='L')
    img.putalpha(alpha_channel)
    img = img.filter(ImageFilter.GaussianBlur(radius=0.5))
    save_brush(img, "brush_crayon")

def create_pencil(size=64):
    """
    铅笔：体积极小，石墨颗粒感，中心实边缘虚
    """
    img = Image.new("RGBA", (size, size), (255, 255, 255, 0))
    canvas = np.zeros((size, size), dtype=np.uint8)
    
    center = size // 2
    for _ in range(size * size): # 随机撒点
        x = random.gauss(center, size/4)
        y = random.gauss(center, size/4)
        if 0 <= x < size and 0 <= y < size:
            # 颗粒非常细碎
            lx, ly = int(x), int(y)
            canvas[ly, lx] = min(255, canvas[ly, lx] + random.randint(50, 200))

    alpha_channel = Image.fromarray(canvas, mode='L')
    img.putalpha(alpha_channel)
    save_brush(img, "brush_pencil")

def create_watercolor(size=512):
    """
    水彩：边缘湿润感（边缘深，中间浅），不规则的浸润扩散
    """
    img = Image.new("RGBA", (size, size), (255, 255, 255, 0))
    # 1. 先画一个大模糊圆
    alpha = Image.new("L", (size, size), 0)
    draw = ImageDraw.Draw(alpha)
    
    margin = 40
    draw.ellipse([margin, margin, size-margin, size-margin], fill=120)
    
    # 2. 模拟“水渍边缘”（Wet Edge）
    # 在边缘画一圈颜色更深的细线
    draw.ellipse([margin+5, margin+5, size-margin-5, size-margin-5], fill=60) # 减淡中间
    
    # 3. 高斯模糊让它晕染开
    alpha = alpha.filter(ImageFilter.GaussianBlur(radius=20))
    
    # 4. 增加一点点有机的不规则纹理（水流动的痕迹）
    alpha_np = np.array(alpha)
    for _ in range(10):
        # 随机画几个模糊的大泡泡增加不均匀感
        x, y = random.randint(0, size), random.randint(0, size)
        r = random.randint(50, 150)
        temp_img = Image.new("L", (size, size), 0)
        temp_draw = ImageDraw.Draw(temp_img)
        temp_draw.ellipse([x-r, y-r, x+r, y+r], fill=random.randint(10, 30))
        temp_img = temp_img.filter(ImageFilter.GaussianBlur(radius=30))
        alpha_np = np.clip(alpha_np + np.array(temp_img), 0, 255)

    img.putalpha(Image.fromarray(alpha_np.astype(np.uint8)))
    save_brush(img, "brush_watercolor")

if __name__ == "__main__":
    create_crayon()
    create_pencil()
    create_watercolor()