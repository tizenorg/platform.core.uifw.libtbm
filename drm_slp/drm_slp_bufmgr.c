/**************************************************************************

xserver-xorg-video-sec

Copyright 2011 Samsung Electronics co., Ltd. All Rights Reserved.

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

#include "config.h"
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include "drm_slp_bufmgr.h"
#include "tbm_bufmgr.h"
#include "tbm_bufmgr_int.h"

drm_slp_bufmgr
drm_slp_bufmgr_init(int fd, void *arg)
{
    tbm_bufmgr bufmgr = NULL;

    bufmgr = tbm_bufmgr_init (fd);
    if (!bufmgr)
    {
        TBM_LOG ("[libdrm_slp:%d]: error bufmgr is null\n", getpid());
        return NULL;
    }

    return (drm_slp_bufmgr)bufmgr;
}

void
drm_slp_bufmgr_destroy(drm_slp_bufmgr bufmgr)
{
    tbm_bufmgr_deinit ((tbm_bufmgr)bufmgr);
}

void
drm_slp_bo_unref(drm_slp_bo bo)
{
    tbm_bo_unref ((tbm_bo)bo);
}

drm_slp_bo
drm_slp_bo_import(drm_slp_bufmgr bufmgr, unsigned int key)
{
    tbm_bo bo = NULL;

    bo = tbm_bo_import ((tbm_bufmgr)bufmgr, key);
    if (!bo)
    {
        TBM_LOG ("[libdrm_slp:%d]: error bo is null\n", getpid());
        return NULL;
    }

    return (drm_slp_bo)bo;
}

unsigned int
drm_slp_bo_map(drm_slp_bo bo, int device, int opt)
{
    tbm_bo_handle bo_handle;
    unsigned int ret = 0;

    bo_handle = tbm_bo_map ((tbm_bo)bo, device, opt);
    if (bo_handle.ptr == NULL)
    {
        TBM_LOG ("[libdrm_slp:%d]: error bo_handle is null\n", getpid());
        return 0;
    }

    switch (device)
    {
        case TBM_DEVICE_DEFAULT:
        case TBM_DEVICE_2D:
        case TBM_DEVICE_3D:
        case TBM_DEVICE_MM:
            ret = (unsigned int)bo_handle.u32;
            break;
        case TBM_DEVICE_CPU:
            ret = (unsigned int)bo_handle.ptr;
            break;
        default:
            TBM_LOG ("[libdrm_slp:%d]: error wrong device type\n", getpid());
            return 0;
    }

    return ret;
}

int
drm_slp_bo_unmap(drm_slp_bo bo, int device)
{
    tbm_bo_unmap ((tbm_bo)bo);

    return 1;
}

