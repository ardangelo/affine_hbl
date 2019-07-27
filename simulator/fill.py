


import numpy as np
import math

SCREEN_WIDTH, SCREEN_HEIGHT = 240, 160

# affine structures
Ps1 = [np.zeros((2, 2)) for i in range(SCREEN_HEIGHT + 1)]
xs1 = [np.zeros((2, 1)) for i in range(SCREEN_HEIGHT + 1)]
# windows: (left, right, top, bottom)
win0h  = [(0, 0, 0, 0) for i in range(SCREEN_HEIGHT + 1)]

# affine display interface
def aff_tx(i, h):
    global Ps1, xs1, win0h

    q = np.array([[i], [h]])
    [[px], [py]] = np.dot(Ps1[h], q) + xs1[h]
    return (px, py)

def in_win(i, h):
    global Ps1, xs1, win0h

    in_win0 = win0h[h][2] <= h and h < win0h[h][3] and win0h[h][0] <= i and i < win0h[h][1]
    return in_win0


# bsp
class Cam:
    def __init__(self, y, z, phi):
        self.a = np.array([[y], [z]])
        self.phi = phi

class Vertex:
    def __init__(self, y, z):
        self.y = y
        self.z = z

class Sector:
    def __init__(self):
        self.color = None
        self.segs = []

class Sidedef:
    def __init__(self, sector):
        self.sector = sector

class Linedef:
    def __init__(self, v1, v2, right, left):
        self.v1 = v1
        self.v2 = v2
        self.right = right
        self.left = left

class Seg:
    def __init__(self, linedef):
        self.v1 = linedef.v1
        self.v2 = linedef.v2
        self.offset = 0
        self.angle = pointtoangleex2(self.v1.y, self.v1.z, self.v2.y, self.v2.z)
        self.linedef = linedef
        self.frontsector = linedef.right.sector
        self.backsector = linedef.left.sector if linedef.left else None
        self.rw_angle1 = 0 # set by addline, used by render

sectors = [
    Sector(), # left above triangle
    Sector(), # right below triangle
    Sector(), # right above triangle
]

# https://www.desmos.com/calculator/uhsyb4xdhf
Vs = [Vertex(y, z) for (y, z) in [
    (-200, 0), (0, 200), (200, 0), (0, -200), # surrounding box
    (40, -140), (-40, -140), (0, -180), # isoceles triangle
    (60, -140), (-60, -140), # split on straight triangle base
    (10, -190) # split on lower triangle leg
]]

linedefs = [
    Linedef(Vs[0], Vs[1], Sidedef(sectors[0]), None),
    Linedef(Vs[1], Vs[2], Sidedef(sectors[0]), None),
    Linedef(Vs[2], Vs[7], Sidedef(sectors[0]), None),
    Linedef(Vs[7], Vs[4], Sidedef(sectors[0]), Sidedef(sectors[2])),
    Linedef(Vs[4], Vs[5], Sidedef(sectors[0]), None), # split 0
    Linedef(Vs[5], Vs[8], Sidedef(sectors[0]), Sidedef(sectors[1])),
    Linedef(Vs[8], Vs[0], Sidedef(sectors[0]), None),

    # 5 -> 8
    Linedef(Vs[5], Vs[6], Sidedef(sectors[1]), None), # split 1
    Linedef(Vs[6], Vs[9], Sidedef(sectors[1]), Sidedef(sectors[2])),
    Linedef(Vs[9], Vs[3], Sidedef(sectors[1]), None),
    Linedef(Vs[3], Vs[8], Sidedef(sectors[1]), None),

    Linedef(Vs[6], Vs[4], Sidedef(sectors[2]), None),
    # 4 -> 7
    Linedef(Vs[7], Vs[9], Sidedef(sectors[2]), None),
    # 9 -> 6
]

# map
cam = Cam(0, 0, np.pi/2 + 0.1)

# precalculate
M7_TOP   = SCREEN_HEIGHT / 2
M7_RIGHT = SCREEN_WIDTH  / 2
M7_D = M7_TOP # for 90deg fov
M7_HFOV = math.atan(float(M7_TOP) / M7_D)
_viewangletoy = {
    math.atan(float(M7_D - h) / M7_TOP): h for h in range(SCREEN_HEIGHT + 1)
    #math.atan((M7_TOP - float(h)) / M7_D) + np.pi / 2: h for h in range(SCREEN_HEIGHT + 1)
}
_ytoviewangle = {
    h: alpha for (alpha, h) in _viewangletoy.items()
}

def ytoviewangle(h):
    clipped = np.clip(h, 0, SCREEN_HEIGHT)
    return _ytoviewangle[clipped]
clipangle = _ytoviewangle[0]
def viewangletoy(alpha):
    clipped = np.clip(alpha, _ytoviewangle[SCREEN_HEIGHT], _ytoviewangle[0])
    nearest = min(_viewangletoy.keys(), key = lambda x: abs(x - clipped))
    return _viewangletoy[nearest]

# scale for current line at given angle
def scalefromglobalangle(rw_distance, rw_normalangle, visangle):
    print ("scalefromglobalangle dist %.2f norm %.2f visang %.2f -> " % (rw_distance, rw_normalangle, visangle)),
    global cam

    anglea = np.pi/2 + (visangle - cam.phi)
    angleb = np.pi/2 + (visangle - rw_normalangle)
    sina = np.sin(anglea)
    sinb = np.sin(angleb)
    num = M7_TOP * sinb
    den = rw_distance * sina
    scale = num / den if den else 100000

    '''
    sinv = np.sin(visangle - rw_normalangle)
    dist = rw_distance / sinv if sinv else np.inf
    cosv = np.cos(cam.phi - visangle)
    z = abs(dist * cosv)
    scale = M7_TOP / z if z else 100000
    '''
    print scale
    return scale

def pointtoangleex2(y1, z1, y, z):
    print ("pointtoangleex2 %d, %d (from %d, %d) " % (y, z, y1, z1)),
    y -= y1
    z -= z1

    # 0 at -y,0; pi/2 at 0,-z; pi at y,0, 3pi/2 at 0,z
    ang = math.atan2(-z, -y)
    #ang = math.atan2(z, y)
    ang = ang + (2*np.pi if ang < 0 else 0)
    print "  camspace %d, %d ang %f" % (y, z, ang)
    return ang

# in camera space
def pointtoangle(y, z):
    global cam
    return pointtoangleex2(cam.a[0][0], cam.a[1][0], y, z)

for (y, z, ang) in [(-1, 0, 0.0), (0, -1, np.pi/2), (1, 0, np.pi), (0, 1, 3*np.pi/2)]:
    #assert(pointtoangle(y, z) == ang)
    x=0

def safe_atan(opp, adj):
    if adj:
        if opp:
            return math.atan(float(opp) / adj)
        return 0 if adj > 0 else math.pi
    elif opp > 0:
        return math.pi / 2
    else:
        return 3 * math.pi / 2

# in camera space
def pointtodist(y, z):
    global cam

    dy = abs(y - cam.a[0][0])
    dz = abs(z - cam.a[1][0])

    if dz > dy:
        dz, dy = dy, dz

    angle = safe_atan(dz, dy) + np.pi/2
    sin_angle = np.sin(angle)
    if sin_angle:
        return dy / sin_angle
    return np.inf

def scale_mat(lam):
    return np.array([[lam, 0], [0, lam]])

def rot_mat(phi):
    return np.array([[np.cos(phi), -np.sin(phi)], [np.sin(phi), np.cos(phi)]])

# bsp render

class Drawseg:
    def __init__(self):
        self.curline = None
        self.y1 = 0
        self.y2 = 0
        self.scale1 = 0
        self.scale2 = 0
        self.scalestep = 0

drawsegs = []

def rendersegloop(drawseg):
    global Ps1
    print "rendersegloop " + str(drawseg) + " from " + str(drawseg.y1) + " to " + str(drawseg.y2)

    scale = drawseg.scale1
    for y in range(drawseg.y1, drawseg.y2 + 1):
        lam = 1 / scale
        Ps1[y] = scale_mat(lam)
        xs1[y] = np.array([[lam * -(SCREEN_WIDTH/2)], [lam * -(SCREEN_HEIGHT/2) + (scale * drawseg.y1)]])
        winwidth = scale * SCREEN_WIDTH
        win0h[y] = ((SCREEN_WIDTH - winwidth) / 2, (SCREEN_WIDTH + winwidth) / 2, 0, SCREEN_HEIGHT)
        scale += drawseg.scalestep
    print


# solid wall render between [start, stop]
def storewallrange(cam, curline, start, stop):
    print "storewallrange %s" % curline

    rw_normalangle = curline.angle + np.pi/2
    rw_normalangle -= 2*np.pi if rw_normalangle >= 2*np.pi else 0
    offsetangle = abs(rw_normalangle - curline.rw_angle1)

    if offsetangle > np.pi/2:
        offsetangle = np.pi/2

    rw_distangle = np.pi/2 - offsetangle
    hyp = pointtodist(curline.v1.y, curline.v1.z)
    sin_distangle = np.sin(rw_distangle)
    rw_distance = hyp * sin_distangle

    drawseg = Drawseg()
    drawseg.y1 = start
    rw_x = start
    drawseg.y2 = stop
    drawseg.curline = curline
    rw_stopx = stop + 1

    # calculate scale at ends and step
    drawseg.scale1 = scalefromglobalangle(
        rw_distance, rw_normalangle,
        cam.phi + ytoviewangle(start))
    rw_scale = drawseg.scale1

    if stop > start:
        drawseg.scale2 = scalefromglobalangle(
        rw_distance, rw_normalangle,
        cam.phi + ytoviewangle(stop))
        drawseg.scalestep = (drawseg.scale2 - rw_scale) / (stop - start)
        rw_scalestep = drawseg.scalestep

    else:
        drawseg.scale2 = drawseg.scale1

    # texture boundaries

    # render
    rendersegloop(drawseg)

    # save sprite clipping info

    drawsegs.append(drawseg)



# solid wall clip
solidsegs = [None for i in range(SCREEN_HEIGHT)]
solidsegs[0] = [-np.inf, -1] # (first, last)
solidsegs[1] = [SCREEN_HEIGHT, np.inf]
ss_newend = 2
def clipsolidwallsegment(cam, drawseg, first, last):
    print "clipsolidwallsegment"
    global solidsegs, ss_newend
    # Find the first range that touches the range (adjacent pixels are touching)
    ss_start = 0
    ss_next = -1
    while solidsegs[ss_start][1] < first - 1:
        ss_start += 1
    if first < solidsegs[ss_start][0]:
        if last < solidsegs[ss_start][0] - 1:
            # Post is entirely visible (above start),
            # so insert a new clippost
            storewallrange(cam, drawseg, first, last)
            ss_next = ss_newend
            ss_newend += 1

            # move all solidsegs up one
            while ss_next != ss_start:
                solidsegs[ss_next] = solidsegs[ss_next - 1]
                ss_next -= 1
            solidsegs[ss_next] = [first, last]
            return
        # wall fragment above ss_start
        storewallrange(cam, drawseg, first, solidsegs[ss_start][0] - 1)
        # adjust clip size
        solidsegs[ss_start][0] = first

    # bottom inside ss_start?
    if last <= solidsegs[ss_start][1]:
        return

    ss_next = ss_start
    frag_after_ss_next = True
    while last >= solidsegs[ss_next + 1][0] - 1:
        # fragment between two existing posts
        storewallrange(cam, drawseg, solidsegs[ss_next][1] + 1, solidsegs[ss_next + 1][0] - 1)
        ss_next += 1

        if last <= solidsegs[ss_next][1]:
            # bottom inside ss_next, adjust clip size
            solidsegs[ss_start][1] = solidsegs[ss_next][1]
            frag_after_ss_next = False
            break

    if frag_after_ss_next:
        storewallrange(cam, drawseg, solidsegs[ss_next][1] + 1, last)
        # adjust clip size
        solidsegs[ss_start][1] = last

    # remove [ss_start+1, ss_next] from clip list, ss_start covers whole area
    if ss_next == ss_start:
        # only covered one post at ss_next, ss_next has "garbage"
        return
    while ss_next != ss_newend:
        ss_next += 1
        # remove post
        ss_start += 1
        solidsegs[ss_start] = solidsegs[ss_next]
    ss_newend = ss_start + 1


# clip, cull or add line seg to visible list
def addline(cam, seg):
    print "addline"
    # curline set to line

    angle1 = pointtoangle(seg.v1.y, seg.v1.z)
    angle2 = pointtoangle(seg.v2.y, seg.v2.z)

    span = angle1 - angle2

    if span >= np.pi:
        print "discarding span %f a1 %f a2 %f" % (span, angle1, angle2)
        return

    seg.rw_angle1 = angle1
    angle1 -= cam.phi
    angle2 -= cam.phi

    print "span %f a1 %f a2 %f" % (span, angle1, angle2)

    '''
    if (angle1 > clipangle) or (angle2 < -clipangle):
        print "completely off edge clipangle %.2f" % clipangle
        return
    angle1 = np.clip(angle1, -clipangle, clipangle)
    angle2 = np.clip(angle2, -clipangle, clipangle)
    '''

    tspan = angle1 + clipangle
    if tspan > 2*clipangle:
        tspan -= 2*clipangle

        if tspan >= span:
            print "tspan %.2f completely off top edge" % tspan
            return

        angle1 = clipangle

    tspan = clipangle - angle2
    if tspan > 2*clipangle:
        tspan -= 2*clipangle

        if tspan >= span:
            print "tspan %.2f completely off bottom edge" % tspan
            return

        angle2 = -clipangle

    #angle1 += np.pi/2
    #angle2 += np.pi/2
    y1 = viewangletoy(angle1)
    y2 = viewangletoy(angle2)

    if y1 == y2:
        print "span single pixel"
        return

    if seg.backsector == None:
        print "clipping %d-%d (%f, %f)" % (y1, y2, angle1, angle2)
        # single sided line
        clipsolidwallsegment(cam, seg, y1, y2 - 1)

def init_sector_segs():
    for ld in linedefs:
        ld.right.sector.segs.append(Seg(ld))
        if ld.left:
            ld.left.sector.segs.append(Seg(ld))

class BspNode:
    def __init__(self, splitter_linedef, right, left):
        self.leaf = False
        self.y = splitter_linedef.v1.y
        self.z = splitter_linedef.v1.z
        self.dy = splitter_linedef.v2.y - self.y
        self.dz = splitter_linedef.v2.z - self.z
        self.right = right
        self.left = left

class BspLeaf:
    def __init__(self, sector):
        self.leaf = True
        self.sector = sector

bsp_root = BspNode(
    linedefs[4],
    BspNode(
        linedefs[7],
        BspLeaf(sectors[1]),
        BspLeaf(sectors[2])),
    BspLeaf(sectors[0]))

def pointonleftside(cam, node):
    y = cam.a[0][0]
    z = cam.a[1][0]
    if node.dz == 0:
        return (node.dy > 0) if (z <= node.z) else (node.dy < 0)
    if node.dy == 0:
        return (node.dz < 0) if (y <= node.y) else (node.dz > 0)
    z -= node.z
    y -= node.y
    if (node.dy ^ node.dz ^ z ^ y) < 0:
        return (node.dy ^ z) < 0
    return (y * node.dz) >= (node.dy * z)

def pointonrightside(cam, root):
    return not pointonleftside(cam, root)

def render_subsector(cam, sector):
    for seg in sector.segs:
        addline(cam, seg)

def renderbspnode(cam, root):
    if root.leaf:
        render_subsector(cam, root.sector)
    else:
        if pointonrightside(cam, root):
            renderbspnode(cam, root.right)
            renderbspnode(cam, root.left)
        else:
            renderbspnode(cam, root.left)
            renderbspnode(cam, root.right)

init_sector_segs()

def m7_hbl():
    global Ps1, xs1, win0h
    global cam, walls
    global solidsegs, drawsegs

    drawsegs = []

    renderbspnode(cam, bsp_root)

    print
    print "rendered %d segs" % len(drawsegs)

if __name__ == '__main__':
    m7_hbl()
