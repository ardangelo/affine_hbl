import pygame
from PIL import Image
import numpy as np

from fill import SCREEN_HEIGHT, SCREEN_WIDTH, m7_hbl, aff_tx, in_win, cam

# load map data
map1_data = np.array(Image.open('gfx/bc1floor.png').convert('RGB'))
map1_W, map1_H, _ = map1_data.shape

# setup pygame
screen = pygame.display.set_mode((SCREEN_WIDTH, SCREEN_HEIGHT))
clock = pygame.time.Clock()

def main_loop():
    running = True
    need_update = True

    while running:
        if need_update:
            m7_hbl()

            for h in range(SCREEN_HEIGHT):
                for i in range(SCREEN_WIDTH):
                    # calculate affine bg 1 pixel
                    (px, py) = aff_tx(i, h)
                    [r1, g1, b1] = np.ndarray.tolist(np.ndarray.flatten(map1_data[int(px) % map1_W, int(py) % map1_H]))

                    # calculate layer based on windows
                    rgb = (r1, g1, b1) if in_win(i, h) else (0, 0, 0)

                    # draw on screen
                    screen.set_at((i, h), rgb)

            need_update = False

        # handle events
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
            elif event.type == pygame.KEYDOWN:
                if event.key == pygame.K_UP:
                    cam.a[1][0] -= 10
                    break
                if event.key == pygame.K_DOWN:
                    cam.a[1][0] += 10
                    break
                if event.key == pygame.K_LEFT:
                    cam.phi -= np.pi/8
                    break
                if event.key == pygame.K_RIGHT:
                    cam.phi += np.pi/8
                    break
                need_update = True

        pygame.display.flip()
        clock.tick(240)

main_loop()

