#ifndef MAIN_H_
#define MAIN_H_

#include <tonc.h>

/* mode 7 constants */
#define PIX_PER_BLOCK 16

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
typedef struct _m7_cam_t {
	VECTOR pos;

	int theta; /* polar angle */
	int phi; /* azimuth angle */
	VECTOR u; /* local x-axis */
	VECTOR v; /* local y-axis */
	VECTOR w; /* local z-axis */

	FIXED fov;
} m7_cam_t;

typedef struct _m7_level_t {
	m7_cam_t *camera;
	u16 *winh; /* window 0 widths */

	BG_AFFINE *bgaff; /* affine parameter array */
	u16 bgcnt; /* BGxCNT for floor */

	int *blocks;
	int blocks_width, blocks_height;
	FIXED pixels_per_block, a_x_range;
	int texture_width, texture_height;
	int *window_extents;
} m7_level_t;

typedef struct {
	FIXED inv_fov;
	FIXED inv_fov_x_ppb;
	FIXED x_cs[SCREEN_HEIGHT];
} m7_precompute;

/* accessible both from main and iwram */
extern m7_level_t floor_level, wall_level;
extern m7_precompute pre;

/* level functions */
void m7_init(m7_level_t *level, m7_cam_t *cam, BG_AFFINE bgaff[], u16 *winh_arr, u16 bgcnt, int bgno);

/* camera functions */
void m7_rotate(m7_cam_t *cam, int theta);
void m7_translate_local(m7_level_t *level, const VECTOR *dir);
void m7_translate_level(m7_level_t *level, const VECTOR *dir);

/* iwram code */
IWRAM_CODE void m7_prep_affines(m7_level_t *level_2, m7_level_t *level_3);
IWRAM_CODE void m7_hbl();

#endif