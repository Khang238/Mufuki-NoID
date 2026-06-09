import pygame #type: ignore
import math

text_cache = {}
def distance(point1, point2):
    dist_x = abs(point1[0] - point2[0])
    dist_y = abs(point1[1] - point2[1])
    return (dist_x ** 2 + dist_y ** 2) ** 0.5
def move_dir(pos, dir, step=1):
    return (cos(dir) * step + pos[0], sin(dir) * step + pos[1])
def get_dir(point1, point2):
    return math.degrees(math.atan2(point2[1] - point1[1], point2[0] - point1[0]))
def RGB_trs(color1, color2, value = 10):
    r1, g1, b1 = color1
    r2, g2, b2 = color2
    r = int(r1 + (r2 - r1) * value / 100)
    g = int(g1 + (g2 - g1) * value / 100)
    b = int(b1 + (b2 - b1) * value / 100)
    return (r, g, b)
def center_text(screen, text, color, pos, size=20, alpha=255):
    global text_cache
    key = (str(text), color, size)
    if key not in text_cache:
        font = pygame.font.Font('font.ttf', size)
        text_surface = font.render(str(text), True, color)
        text_cache[key] = text_surface
    text_surface_alpha = text_cache[key].copy()
    text_surface_alpha.set_alpha(alpha)
    text_rect = text_surface_alpha.get_rect(center=pos)
    screen.blit(text_surface_alpha, text_rect)
    return (text_rect.width, text_rect.height)

def left_text(screen, text, color, pos, size=40, alpha=255, draw=True):
    global text_cache
    key = (text, color, size)
    if key not in text_cache:
        font = pygame.font.Font('font.ttf', size)
        text_surface = font.render(str(text), True, color)
        text_cache[key] = text_surface
    text_surface_alpha = text_cache[key].copy()
    text_surface_alpha.set_alpha(alpha)
    text_rect = text_surface_alpha.get_rect(topleft=pos)
    if draw:
        screen.blit(text_surface_alpha, text_rect)
    return (text_rect.width, text_rect.height)

def sin(deg):
    return math.sin(math.radians(deg))

def cos(deg):
    return math.cos(math.radians(deg))

def right_text(screen, text, color, pos, size=40, alpha=255, draw=True):
    global text_cache
    key = (text, color, size)
    if key not in text_cache:
        font = pygame.font.Font('font.ttf', size)
        text_surface = font.render(str(text), True, color)
        text_cache[key] = text_surface
    text_surface_alpha = text_cache[key].copy()
    text_surface_alpha.set_alpha(alpha)
    text_rect = text_surface_alpha.get_rect(topright=pos)
    if draw:
        screen.blit(text_surface_alpha, text_rect)
    return (text_rect.width, text_rect.height)