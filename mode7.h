#ifndef MAIN_H_
#define MAIN_H_

#include <tonc.h>

/* mode 7 constants */
#define PIX_PER_BLOCK 16

#define M7_OBJ_COUNT 32

#define M7_D 160 /* focal length */
#define M7_D_SHIFT 8 /* focal shift */
#define M7_RENORM_SHIFT 2 /* renormalization shift */

#define M7_LEFT   (-120) /* viewport left */
#define M7_RIGHT    120  /* viewport right */
#define M7_TOP       80  /* viewport top */
#define M7_BOTTOM  (-80) /* viewport bottom */
#define M7_NEAR      24  /* near plane for objects */
#define M7_FAR_OBJ  512  /* far plane for objects */
#define M7_FAR_BG   768  /* far plane for floor */

/* mode 7 types */
class M7Camera {
public:
	VECTOR pos;

	int theta; /* polar angle */
	int phi; /* azimuth angle */
	VECTOR u; /* local x-axis */
	VECTOR v; /* local y-axis */
	VECTOR w; /* local z-axis */

	FIXED fov;

	M7Camera(FIXED f);
	void rotate(FIXED th);
};

class M7Map {
public:
	u16 winh[SCREEN_HEIGHT + 1]; /* window 0 widths */
	BG_AFFINE bgaff[SCREEN_HEIGHT + 1]; /* affine parameter array */
	u16 bgcnt; /* BGxCNT for floor */

	const int *blocks;
	int blocks_width, blocks_height, blocks_depth;
	FIXED *extent_widths, *extent_offs;

	FIXED texture_width, texture_height, texture_depth;

	M7Map(u16 bgc, const int *bl, int bw, int bh, int bd, FIXED *ew,
		FIXED *eo, FIXED fov, const FIXED *extents);
};

class M7Level {
public:
	M7Camera *cam;
	M7Map* maps[2];

	M7Level(M7Camera *c, M7Map *m1, M7Map *m2);
	void translateLocal(const VECTOR *dir);

	IWRAM_CODE void HBlank();
	IWRAM_CODE void prepAffines();
	IWRAM_CODE void applyAffines(int vc);
};

typedef struct {
	FIXED inv_fov;
	FIXED inv_fov_x_ppb;
	FIXED x_cs[SCREEN_HEIGHT];
} m7_precompute;

/* accessible both from main and iwram */
extern m7_precompute pre;
extern M7Level fanLevel;

IWRAM_CODE void m7_hbl();

#endif