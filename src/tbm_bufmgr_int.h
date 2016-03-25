/**************************************************************************

libtbm

Copyright 2012 Samsung Electronics co., Ltd. All Rights Reserved.

Contact: SooChan Lim <sc1.lim@samsung.com>, Sangjin Lee <lsj119@samsung.com>
Boram Park <boram1288.park@samsung.com>, Changyeon Lee <cyeon.lee@samsung.com>

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sub license, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial portions
of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

#ifndef _TBM_BUFMGR_INT_H_
#define _TBM_BUFMGR_INT_H_

#include <unistd.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <tbm_bufmgr.h>
#include <tbm_surface.h>
#include <tbm_surface_internal.h>
#include <tbm_bufmgr_backend.h>
#include <tbm_surface_queue.h>

/* check condition */
#define TBM_RETURN_IF_FAIL(cond) {\
	if (!(cond)) {\
		TBM_LOG("[%s] : '%s' failed.\n", __func__, #cond);\
		return;\
	} \
}
#define TBM_RETURN_VAL_IF_FAIL(cond, val) {\
	if (!(cond)) {\
		TBM_LOG("[%s] : '%s' failed.\n", __func__, #cond);\
		return val;\
	} \
}

/* check flags */
#define RETURN_CHECK_FLAG(cond) {\
	if ((cond)) {\
		return;\
	} \
}
#define RETURN_VAL_CHECK_FLAG(cond, val) {\
	if ((cond)) {\
		return val;\
	} \
}

/* check validation */
#define TBM_BUFMGR_IS_VALID(mgr) (mgr)
#define TBM_BO_IS_VALID(bo) (bo && \
			    TBM_BUFMGR_IS_VALID(bo->bufmgr) && \
			    bo->item_link.next && \
			    bo->item_link.next->prev == &bo->item_link)
#define TBM_SURFACE_IS_VALID(surf) (surf && \
				   TBM_BUFMGR_IS_VALID(surf->bufmgr) && \
				   surf->item_link.next && \
				   surf->item_link.next->prev == &surf->item_link)

#define TBM_DEBUG(fmt, ...)   fprintf(stderr, "[TBM:DEBUG(%d)] " fmt, getpid(), ##__VA_ARGS__)
#define TBM_LOG(fmt, ...)  fprintf(stderr, "[TBM(%d):%s] " fmt, getpid(), __func__, ##__VA_ARGS__)

struct list_head {
	struct list_head *prev;
	struct list_head *next;
};

/**
 * @brief tbm_bo : buffer object of Tizen Buffer Manager
 */
struct _tbm_bo {
	tbm_bufmgr bufmgr;			/* tbm buffer manager */

	int ref_cnt;				/* ref count of bo */

	int flags;					/* TBM_BO_FLAGS :bo memory type */

	struct list_head user_data_list;	/* list of the user_date in bo */

	void *priv;					/* bo private */

	struct list_head item_link;	/* link of bo */

	tbm_surface_h surface; /* tbm_surface */

	int lock_cnt;				/* lock count of bo */

	unsigned int map_cnt;		/* device map count */
};

/**
 * @brief tbm_bufmgr : structure for tizen buffer manager
 *
 */
struct _tbm_bufmgr {
	pthread_mutex_t lock;		/* mutex lock */

	int ref_count;				/*reference count */

	int fd;						/* bufmgr fd */

	int lock_type;				/* lock_type of bufmgr */

	struct list_head bo_list;	/* list of bos belonging to bufmgr */

	struct list_head surf_list;	/* list of surfaces belonging to bufmgr */

	void *module_data;

	tbm_bufmgr_backend backend;	/* bufmgr backend */
};

/**
 * @brief tbm_surface : structure for tizen buffer surface
 *
 */
struct _tbm_surface {
	tbm_bufmgr bufmgr;			/* tbm buffer manager */

	tbm_surface_info_s info;	/* tbm surface information */

	int flags;

	int num_bos;				/* the number of buffer objects */

	tbm_bo bos[4];

	int num_planes;				/* the number of buffer objects */

	int planes_bo_idx[TBM_SURF_PLANE_MAX];

	int refcnt;

	unsigned int debug_pid;

	struct list_head item_link; /* link of surface */

	struct list_head user_data_list;	/* list of the user_date in surface */
};

typedef struct {
	unsigned long key;
	void *data;
	tbm_data_free free_func;

	/* link of user_data */
	struct list_head item_link;
} tbm_user_data;

int _tbm_bo_set_surface(tbm_bo bo, tbm_surface_h surface);

/* functions for mutex */
int tbm_surface_internal_get_info(tbm_surface_h surface, int opt,
				  tbm_surface_info_s *info, int map);
void tbm_surface_internal_unmap(tbm_surface_h surface);
unsigned int tbm_surface_internal_get_width(tbm_surface_h surface);
unsigned int tbm_surface_internal_get_height(tbm_surface_h surface);
tbm_format tbm_surface_internal_get_format(tbm_surface_h surface);
unsigned int _tbm_surface_internal_get_debug_pid(tbm_surface_h surface);
char *_tbm_surface_internal_format_to_str(tbm_format format);

tbm_user_data *user_data_lookup(struct list_head *user_data_list,
				unsigned long key);
tbm_user_data *user_data_create(unsigned long key,
				tbm_data_free data_free_func);
void user_data_delete(tbm_user_data *user_data);

#endif							/* _TBM_BUFMGR_INT_H_ */
