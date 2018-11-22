#ifndef MODE7_H_
#define MODE7_H_

#include <tonc.h>
#include <Chip/Unknown/Nintendo/GBA.hpp>

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

/* mode 7 classes */

/* https://www.embedded.com/print/4438679
The criteria for a static initializer to be allowed for a class are:
- The class must have no base classes.
- It must have no constructor.
- It must have no virtual functions.
- It must have no private or protected members.
- In addition, we should also require that all member functions of a ROMable class be const.
*/

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

	/* background control information */
	const int charBaseBlock, screenBaseBlock, priority;
	const Kvasir::BackgroundSizes bgSize;

	const int *blocks;
	int blocksWidth, blocksHeight, blocksDepth;
	FIXED *extentWidths, *extentOffs;

	FIXED textureWidth, textureHeight, textureDepth;

	M7Map(int cbb, int sbb, int prio, Kvasir::BackgroundSizes size,
		const int *bl, int bw, int bh, int bd,
		FIXED *ew, FIXED *eo, const FIXED *extents, FIXED fov);
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

/* general structures  */

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