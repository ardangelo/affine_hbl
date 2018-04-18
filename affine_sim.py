import pygame
from PIL import Image
import numpy as np
import math

W, H = 240, 160

# load map data
map_data = np.array(Image.open('gfx/bc1floor.png').convert('RGB'))
map_W, map_H, _ = map_data.shape

# init "software" structures
class BGAffine:
    def __init__(self):
        self.pa = 0
        self.pb = 0
        self.pc = 0
        self.pd = 0
        self.x = 0
        self.y = 0
    def P(self):
        return np.array([[self.pa, self.pb], [self.pc, self.pd]])
    def dx(self):
        return np.array([[self.x], [self.y]])
bgaff = [BGAffine() for i in range(H + 1)]

# mode 7 structures
M7_D = 160.0
class Vector:
    def __init__(self, x=0.0, y=0.0, z=0.0):
        self.x = x
        self.y = y
        self.z = z
cam_pos = Vector(0.0, 32.0, 0.0)
cam_phi = 0.0

# calculate affine matrices
def m7_hbl():
    global bgaff, cam_pos, cam_phi

    for h in range(H):
        if h == 0:
            lam = 0.0
        else:
            lam = cam_pos.y / float(h)
        lam_cos_phi = lam * math.cos(cam_phi)
        lam_sin_phi = lam * math.sin(cam_phi)

        bgaff[h].pa = lam_cos_phi
        bgaff[h].pd = lam_sin_phi

        lxr = 120.0 * lam_cos_phi
        lyr = M7_D * lam_sin_phi
        bgaff[h].x = cam_pos.x - lxr + lyr

        lxr = 120.0 * lam_sin_phi
        lyr = M7_D * lam_cos_phi
        bgaff[h].y = cam_pos.z - lxr - lyr

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
                # apply affine transform
                q = np.array([[i], [h]])
                p = bgaff[h].dx() + bgaff[h].P() @ q
                [px, py] = np.ndarray.tolist(np.ndarray.flatten(p))
                rgb = np.ndarray.tolist(np.ndarray.flatten(map_data[int(px) % map_W, int(py) % map_H]))
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