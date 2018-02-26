#ifndef MAIN_H_
#define MAIN_H_

#include <tonc.h>

/* mode 7 constants */
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

	enum {
		CAM_NORMAL, CAM_ZOOMIN, CAM_ZOOMED, CAM_ZOOMOUT
	} state;
	int theta; /* polar angle */
	int phi; /* azimuth angle */
	VECTOR u; /* local x-axis */
	VECTOR v; /* local y-axis */
	VECTOR w; /* local z-axis */
} m7_cam_t;

typedef struct _m7_level_t {
	m7_cam_t *camera;
	BG_AFFINE *bgaff; /* affine parameter array */
	int horizon; /* horizon scanline */
	u16 bgcnt_sky; /* BGxCNT for sky */
	u16 bgcnt_floor; /* BGxCNT for floor */
} m7_level_t;

/* accessible both from main and iwram */
extern m7_level_t m7_level;

/* level functions */
void m7_init(m7_level_t *level, m7_cam_t *cam, BG_AFFINE *aff_arr, u16 skycnt, u16 floorcnt);
void m7_prep_horizon(m7_level_t *level);
void m7_update_sky(const m7_level_t *level);

/* camera functions */
void m7_rotate(m7_cam_t *cam, int theta, int phi);
void m7_translate(m7_cam_t *cam, const VECTOR *dir);

/* iwram code */
IWRAM_CODE void m7_prep_affines(m7_level_t *level);
IWRAM_CODE void m7_hbl();

#endif