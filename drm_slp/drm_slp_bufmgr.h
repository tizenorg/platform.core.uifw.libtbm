/**************************************************************************

xserver-xorg-video-sec

Copyright 2012 Samsung Electronics co., Ltd. All Rights Reserved.

Contact: SooChan Lim <sc1.lim@samsung.com>, Sangjin Lee <lsj119@samsung.com>

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

#ifndef _DRM_SLP_BUFMGR_H_
#define _DRM_SLP_BUFMGR_H_

#include "tbm_bufmgr.h"

typedef struct _drm_slp_bo * drm_slp_bo;
typedef struct _drm_slp_bufmgr * drm_slp_bufmgr;

/* Functions for buffer mnager */
drm_slp_bufmgr drm_slp_bufmgr_init(int fd, void * arg);
void           drm_slp_bufmgr_destroy(drm_slp_bufmgr bufmgr);

/*Functions for bo*/
void         drm_slp_bo_unref(drm_slp_bo bo);
drm_slp_bo drm_slp_bo_import(drm_slp_bufmgr bufmgr, unsigned int key);
unsigned int drm_slp_bo_map(drm_slp_bo bo, int device, int opt);
int          drm_slp_bo_unmap(drm_slp_bo bo, int device);

#endif /* _DRM_SLP_BUFMGR_H_ */
