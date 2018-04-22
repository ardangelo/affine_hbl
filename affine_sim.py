import pygame
from PIL import Image
import numpy as np
import math

from level import blocks, TILE_SIZE

SCREEN_WIDTH, SCREEN_HEIGHT = 240, 160

# load map data
map1_data = np.array(Image.open('gfx/bc1floor.png').convert('RGB'))
map1_W, map1_H, _ = map1_data.shape
map2_data = np.array([[[0, 0, 0]]])
map2_W, map2_H, _ = (1, 1, 3)

# init "software" structures
class BGAffine:
    def __init__(self):
        self.pa = 1 << 8
        self.pb = 0
        self.pc = 0
        self.pd = 1 << 8
        self.x = 0
        self.y = 0
    def P(self):
        return np.array([[float(self.pa >> 8), float(self.pb >> 8)], [float(self.pc >> 8), float(self.pd >> 8)]])
    def dx(self):
        return np.array([[float(self.x >> 8)], [float(self.y >> 8)]])
bgaff1 = [BGAffine() for i in range(SCREEN_HEIGHT + 1)]
bgaff2 = [BGAffine() for i in range(SCREEN_HEIGHT + 1)]

# windows: (left, right, top, bottom)
win0h  = [(0, 0, 0, 0) for i in range(SCREEN_HEIGHT + 1)]
win1h  = [(0, 0, 0, 0) for i in range(SCREEN_HEIGHT + 1)]

# mode 7 structures
M7_D = 160
M7_TOP = SCREEN_HEIGHT / 2
M7_BOTTOM = -M7_TOP
M7_RIGHT = SCREEN_WIDTH / 2
M7_LEFT = -M7_RIGHT
class Vector:
    def __init__(self, x=0, y=0, z=0):
        self.x = x
        self.y = y
        self.z = z
cam_pos = Vector(22 << 8, 0 << 8, 12 << 8)
cam_u = Vector(1 << 8, 0, 0)
cam_v = Vector(0, -1 << 8, 0)
cam_w = Vector(0,  0, 1 << 0)

# per-hblank calculations
def m7_hbl():
    global bgaff, cam_pos, cam_u, cam_v, cam_w

    a_x = cam_pos.x
    a_y = cam_pos.y
    a_z = cam_pos.z

    cos_theta = cam_v.y
    sin_theta = cam_w.y

    plane_x = 0 << 8
    plane_z = 1 << 8

    for h in range(SCREEN_HEIGHT):
        x_c = int(2 * (h << 8) / SCREEN_HEIGHT - (1 << 8))

        dir_x = cos_theta + ((plane_x * x_c) >> 8)
        if (dir_x == 0):
            dir_x = 1
        dir_z = sin_theta + ((plane_z * x_c) >> 8)
        if (dir_z == 0):
            dir_z = 1

        map_x = a_x >> 8;
        map_z = a_z >> 8;

        delta_dist_x = int(abs((1 << 16) / dir_x))
        delta_dist_z = int(abs((1 << 16) / dir_z))

        hit = 0
        step_x = 0
        step_z = 0
        side_dist_x = 0
        side_dist_z = 0

        if (dir_x < 0):
            step_x = -1
            side_dist_x = ((a_x - (map_x << 8)) << 8) / dir_x
        else:
            step_x = 1
            side_dist_x = (((map_x << 8) + (1 << 8) - a_x) << 8) / dir_x

        if (dir_z < 0):
            step_z = -1
            side_dist_z = ((a_z - (map_z << 8)) << 8) / dir_z
        else:
            step_z = 1
            side_dist_z = (((map_z << 8) + (1 << 8) - a_z) << 8) / dir_z

        side = 0
        while (not hit):
            if (side_dist_x < side_dist_z):
                side_dist_x += delta_dist_x
                map_x += step_x
                side = 0
            else:
                side_dist_z += delta_dist_z
                map_z += step_z
                side = 1

            if (blocks[map_x][map_z] > 0):
                hit = 1

        perp_wall_dist = 0
        if (side == 0):
            perp_wall_dist = (int((map_x << 8) - a_x + ((1 - step_x) << 8) / 2) << 8) / dir_x
        else:
            perp_wall_dist = (int((map_z << 8) - a_z + ((1 - step_z) << 8) / 2) << 8) / dir_z
        if (perp_wall_dist == 0):
            perp_wall_dist = 1

        line_height = int((SCREEN_WIDTH << 16) / perp_wall_dist) >> 8
        draw_start = -line_height / 2 + SCREEN_WIDTH / 2
        if (draw_start < 0): 
            draw_start = 0
        draw_end = line_height / 2 + SCREEN_WIDTH / 2
        if (draw_end >= SCREEN_WIDTH):
            draw_end = SCREEN_WIDTH

        # calculate affine matrix
        bgaff1[h].pa = int(perp_wall_dist)
        bgaff1[h].pd = int(perp_wall_dist)

        # calculate window boundaries
        win0h[h] = (draw_start, draw_end, 0, SCREEN_HEIGHT)

# start game loop
screen = pygame.display.set_mode((SCREEN_WIDTH, SCREEN_HEIGHT))
clock = pygame.time.Clock()
running = True
need_update = True

while running:
    if need_update:
        m7_hbl()

        for h in range(SCREEN_HEIGHT):
            for i in range(SCREEN_WIDTH):
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
                cam_pos.z -= 1 << 8
            if event.key == pygame.K_DOWN:
                cam_pos.z += 1 << 8
            if event.key == pygame.K_LEFT:
                cam_pos.x -= 1 << 8
            if event.key == pygame.K_RIGHT:
                cam_pos.x += 1 << 8
            need_update = True

    pygame.display.flip()
    clock.tick(240)