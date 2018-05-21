#ifndef MODE7_H_
#define MODE7_H_

#include <tonc.h>

#include "units.hpp"

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

class M7Camera {
public:
	Vector<FixedPixel> pos;

	FixedAngle theta; /* polar angle */
	FixedAngle phi; /* azimuth angle */
	Vector<FixedPixel> u; /* local x-axis */
	Vector<FixedPixel> v; /* local y-axis */
	Vector<FixedPixel> w; /* local z-axis */

	const FixedPixel& fov;

	M7Camera(const FixedPixel& f);
	void rotate(const FixedAngle& th);
};

class M7Map {
public:
	u16 winh[SCREEN_HEIGHT + 1]; /* window parameter array */
	BG_AFFINE bgaff[SCREEN_HEIGHT + 1]; /* affine parameter array */
	const u16 bgcnt; /* BGxCNT for floor */

	const int *blocks;
	const Vector<Block> blocksDim;

	const Block height;
	FixedPixel *extentWidths;
	FixedPixel *extentOffs;

	Vector<FixedPixel> textureDim;

	M7Map(u16 bgc, const int *bl, const Vector<Block>& bD,
		const Block *extents,
		FixedPixel *eW, FixedPixel *eO, /* pointer to statically allocated storage */
		const FixedPixel& fov);
};

class M7Level {
public:
	M7Camera& cam;
	const M7Map* maps[2];

	M7Level(M7Camera& c, const M7Map *m1, const M7Map *m2);
	void translateLocal(const Vector<FixedPixel>& dir);

	IWRAM_CODE void HBlank();
	IWRAM_CODE void prepAffines();
	IWRAM_CODE void applyAffines(int vc);
};

/* raycasting classes */

class Ray {
private:
	const Vector<FixedPixel>& origin;

public:
	enum sides {N, S, E, W};
	const Point<FixedPixel> dist_0, deltaDist, ray, invRay;
	const Point<Block> map_0, deltaMap;

	IWRAM_CODE Ray(const Vector<FixedPixel>& origin, int h);

	typedef struct {
		enum sides sideHit;
		FixedPixel perpWallDist;
		Point<FixedPixel> dist;
		Point<Block> map;
	} Casted;

	IWRAM_CODE const std::optional<Casted>& cast(const M7Map& map);
};

/* general structures */

typedef struct {
	FixedPixel inv_fov;
	FixedPixel inv_fov_x_ppb;
	FixedPixel x_cs[SCREEN_HEIGHT];
} M7Precompute;

/* accessible both from main and iwram */
extern M7Precompute pre;
extern M7Level fanLevel;
IWRAM_CODE void m7_hbl();

#endif