from PIL import Image
import numpy as np

# load map data
map_data = np.array(Image.open('gfx/bc1floor.png').convert('RGB'))
map_W, map_H, _ = map_data.shape

# init "hardware" structures
W, H = 240, 160
screen_data = np.zeros((H, W, 3), dtype=np.uint8)

P  = np.zeros((2, 2))
dx = np.zeros((2, 1))

# init "software" structures
class BGAffine:
    def __init__(self):
        self.pa = 0
        self.pb = 0
        self.pc = 0
        self.pd = 0
        self.dx = 0
        self.dy = 0
    def m_P(self):
        return np.array([[self.pa, self.pb], [self.pc, self.pd]])
    def v_dx(self):
        return np.array([[self.dx], [self.dy]])
bgaff = [BGAffine() for i in range(H + 1)]

# calculate affine matrices
for h in range(H):
    bgaff[h].pa = 1.0 + (h / H)
    bgaff[h].pd = 1.0 + (h / H)

# apply affine transform
for h in range(H):
    for i in range(W):
        v_q = np.array([[i], [h]])
        v_p = bgaff[h].v_dx() + bgaff[h].m_P() @ v_q
        [px, py] = np.ndarray.tolist(np.ndarray.flatten(v_p))
        screen_data[h, i] = map_data[int(px) % map_W, int(py) % map_H]

# display screen data
img = Image.fromarray(screen_data, 'RGB')
img.show()