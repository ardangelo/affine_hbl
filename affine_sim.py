import pygame
from PIL import Image
import numpy as np
import math

W, H = 240, 160

# load map data
map1_data = np.array(Image.open('gfx/bc1floor.png').convert('RGB'))
map1_W, map1_H, _ = map1_data.shape
map2_data = np.array([[[0, 0, 0]]])
map2_W, map2_H, _ = (1, 1, 3)

# init "software" structures
class BGAffine:
    def __init__(self):
        self.pa = 1
        self.pb = 0
        self.pc = 0
        self.pd = 1
        self.x = 0
        self.y = 0
    def P(self):
        return np.array([[self.pa, self.pb], [self.pc, self.pd]])
    def dx(self):
        return np.array([[self.x], [self.y]])
bgaff1 = [BGAffine() for i in range(H + 1)]
bgaff2 = [BGAffine() for i in range(H + 1)]

# windows: (left, right, top, bottom)
win0h  = [(0, 0, 0, 0) for i in range(H + 1)]
win1h  = [(0, 0, 0, 0) for i in range(H + 1)]

# mode 7 structures
M7_D = 160.0
M7_TOP = H / 2
M7_BOTTOM = -M7_TOP
M7_RIGHT = W / 2
M7_LEFT = -M7_RIGHT
class Vector:
    def __init__(self, x=0.0, y=0.0, z=0.0):
        self.x = x
        self.y = y
        self.z = z
cam_pos = Vector(0.0, 32.0, 0.0)
cam_u = Vector(1.0, 0.0, 0.0)
cam_v = Vector(0.0, 1.0, 0.0)
cam_w = Vector(0.0, 0.0, 1.0)

def level_heightmap(z):
    level0 = 0.0
    end0   = 0x032
    start1 = 0x064
    level1 = 0.0
    slope = (level1 - level0) / (start1 - end0)

    z = z % H
    y = 0
    if z < end0:
        y = level0
    elif z > start1:
        y = level1
    else:
        y = slope * (z - end0)

    return y

# calculate affine matrices
def m7_hbl():
    global bgaff, cam_pos, cam_phi

    a_x = cam_pos.x
    a_y = cam_pos.y
    a_z = cam_pos.z

    cos_phi = cam_u.x
    sin_phi = cam_u.z
    cos_theta = cam_v.y
    sin_theta = cam_w.y

    for h in range(H):
        # calculate affine matrix
        yb = (h - M7_TOP) * cos_theta + M7_D * sin_theta
        if yb == 0:
            lam = 0
        else:
            lam = a_y / yb

        lam_cos_phi = lam * cos_phi
        lam_sin_phi = lam * sin_phi

        bgaff1[h].pa = lam_cos_phi
        bgaff1[h].pc = lam_sin_phi

        zb = (h - M7_TOP) * sin_theta - M7_D * cos_theta
        bgaff1[h].x = a_x + lam_cos_phi * M7_LEFT - lam_sin_phi * zb
        bgaff1[h].y = a_z + lam_sin_phi * M7_LEFT + lam_cos_phi * zb

        # calculate window boundaries
        win0h[h] = ((W - h) / 2, (W + h) / 2, 0, 160)

# start game loop
screen = pygame.display.set_mode((W, H))
clock = pygame.time.Clock()
running = True
need_update = True

while running:
    if need_update:
        m7_hbl()

        for h in range(H):
            for i in range(W):
                # calculate affine bg 1 pixel
                q = np.array([[i], [h]])
                p = bgaff1[h].dx() + bgaff1[h].P() @ q
                [px, py] = np.ndarray.tolist(np.ndarray.flatten(p))
                [r1, g1, b1] = np.ndarray.tolist(np.ndarray.flatten(map1_data[int(px) % map1_W, int(py) % map1_H]))

                # calculate affine bg 2 pixel
                q = np.array([[i], [h]])
                p = bgaff2[h].dx() + bgaff2[h].P() @ q
                [px, py] = np.ndarray.tolist(np.ndarray.flatten(p))
                [r2, g2, b2] = np.ndarray.tolist(np.ndarray.flatten(map2_data[int(px) % map2_W, int(py) % map2_H]))

                # calculate layer based on windows
                in_win0 = win0h[h][2] <= h and h < win0h[h][3] and win0h[h][0] <= i and i < win0h[h][1]
                in_win1 = win1h[h][2] <= h and h < win1h[h][3] and win1h[h][0] <= i and i < win1h[h][1]
                in_win = in_win0 or in_win1
                out_win = not in_win
                rgb = (r1, g1, b1) if in_win else (r2, g2, b2)

                # draw on screen
                screen.set_at((i, h), rgb)

        need_update = False

    # handle events
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            running = False
        elif event.type == pygame.KEYDOWN:
            if event.key == pygame.K_UP:
                cam_pos.z -= 1.0
            if event.key == pygame.K_DOWN:
                cam_pos.z += 1.0

    pygame.display.flip()
    clock.tick(240)