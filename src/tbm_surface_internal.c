/**************************************************************************

libtbm

Copyright 2014 Samsung Electronics co., Ltd. All Rights Reserved.

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

#include "config.h"
#include "tbm_bufmgr.h"
#include "tbm_bufmgr_int.h"
#include "tbm_surface_internal.h"
#include "list.h"

static tbm_bufmgr g_surface_bufmgr = NULL;
struct list_head g_surface_list; /* list of surfaces belonging to bufmgr */

static int _tbm_surface_internal_get_num_planes (tbm_format format)
{
    int num_planes = 0;

    switch (format)
    {
        case TBM_FORMAT_C8:
        case TBM_FORMAT_RGB332:
        case TBM_FORMAT_BGR233:
        case TBM_FORMAT_XRGB4444:
        case TBM_FORMAT_XBGR4444:
        case TBM_FORMAT_RGBX4444:
        case TBM_FORMAT_BGRX4444:
        case TBM_FORMAT_ARGB4444:
        case TBM_FORMAT_ABGR4444:
        case TBM_FORMAT_RGBA4444:
        case TBM_FORMAT_BGRA4444:
        case TBM_FORMAT_XRGB1555:
        case TBM_FORMAT_XBGR1555:
        case TBM_FORMAT_RGBX5551:
        case TBM_FORMAT_BGRX5551:
        case TBM_FORMAT_ARGB1555:
        case TBM_FORMAT_ABGR1555:
        case TBM_FORMAT_RGBA5551:
        case TBM_FORMAT_BGRA5551:
        case TBM_FORMAT_RGB565:
        case TBM_FORMAT_BGR565:
        case TBM_FORMAT_RGB888:
        case TBM_FORMAT_BGR888:
        case TBM_FORMAT_XRGB8888:
        case TBM_FORMAT_XBGR8888:
        case TBM_FORMAT_RGBX8888:
        case TBM_FORMAT_BGRX8888:
        case TBM_FORMAT_ARGB8888:
        case TBM_FORMAT_ABGR8888:
        case TBM_FORMAT_RGBA8888:
        case TBM_FORMAT_BGRA8888:
        case TBM_FORMAT_XRGB2101010:
        case TBM_FORMAT_XBGR2101010:
        case TBM_FORMAT_RGBX1010102:
        case TBM_FORMAT_BGRX1010102:
        case TBM_FORMAT_ARGB2101010:
        case TBM_FORMAT_ABGR2101010:
        case TBM_FORMAT_RGBA1010102:
        case TBM_FORMAT_BGRA1010102:
        case TBM_FORMAT_YUYV:
        case TBM_FORMAT_YVYU:
        case TBM_FORMAT_UYVY:
        case TBM_FORMAT_VYUY:
        case TBM_FORMAT_AYUV:
            num_planes = 1;
            break;
        case TBM_FORMAT_NV12:
        case TBM_FORMAT_NV21:
        case TBM_FORMAT_NV16:
        case TBM_FORMAT_NV61:
            num_planes = 2;
            break;
        case TBM_FORMAT_YUV410:
        case TBM_FORMAT_YVU410:
        case TBM_FORMAT_YUV411:
        case TBM_FORMAT_YVU411:
        case TBM_FORMAT_YUV420:
        case TBM_FORMAT_YVU420:
        case TBM_FORMAT_YUV422:
        case TBM_FORMAT_YVU422:
        case TBM_FORMAT_YUV444:
        case TBM_FORMAT_YVU444:
            num_planes = 3;
            break;

        default :
            break;
    }

    return num_planes;
}


static int _tbm_surface_internal_get_bpp (tbm_format format)
{
    int bpp = 0;

    switch (format)
    {
        case TBM_FORMAT_C8:
        case TBM_FORMAT_RGB332:
        case TBM_FORMAT_BGR233:
            bpp = 8;
            break;
        case TBM_FORMAT_XRGB4444:
        case TBM_FORMAT_XBGR4444:
        case TBM_FORMAT_RGBX4444:
        case TBM_FORMAT_BGRX4444:
        case TBM_FORMAT_ARGB4444:
        case TBM_FORMAT_ABGR4444:
        case TBM_FORMAT_RGBA4444:
        case TBM_FORMAT_BGRA4444:
        case TBM_FORMAT_XRGB1555:
        case TBM_FORMAT_XBGR1555:
        case TBM_FORMAT_RGBX5551:
        case TBM_FORMAT_BGRX5551:
        case TBM_FORMAT_ARGB1555:
        case TBM_FORMAT_ABGR1555:
        case TBM_FORMAT_RGBA5551:
        case TBM_FORMAT_BGRA5551:
        case TBM_FORMAT_RGB565:
        case TBM_FORMAT_BGR565:
            bpp = 16;
            break;
        case TBM_FORMAT_RGB888:
        case TBM_FORMAT_BGR888:
            bpp = 24;
            break;
        case TBM_FORMAT_XRGB8888:
        case TBM_FORMAT_XBGR8888:
        case TBM_FORMAT_RGBX8888:
        case TBM_FORMAT_BGRX8888:
        case TBM_FORMAT_ARGB8888:
        case TBM_FORMAT_ABGR8888:
        case TBM_FORMAT_RGBA8888:
        case TBM_FORMAT_BGRA8888:
        case TBM_FORMAT_XRGB2101010:
        case TBM_FORMAT_XBGR2101010:
        case TBM_FORMAT_RGBX1010102:
        case TBM_FORMAT_BGRX1010102:
        case TBM_FORMAT_ARGB2101010:
        case TBM_FORMAT_ABGR2101010:
        case TBM_FORMAT_RGBA1010102:
        case TBM_FORMAT_BGRA1010102:
        case TBM_FORMAT_YUYV:
        case TBM_FORMAT_YVYU:
        case TBM_FORMAT_UYVY:
        case TBM_FORMAT_VYUY:
        case TBM_FORMAT_AYUV:
            bpp = 32;
            break;
        case TBM_FORMAT_NV12:
        case TBM_FORMAT_NV21:
            bpp = 12;
            break;
        case TBM_FORMAT_NV16:
        case TBM_FORMAT_NV61:
            bpp = 16;
            break;
        case TBM_FORMAT_YUV410:
        case TBM_FORMAT_YVU410:
            bpp = 9;
            break;
        case TBM_FORMAT_YUV411:
        case TBM_FORMAT_YVU411:
        case TBM_FORMAT_YUV420:
        case TBM_FORMAT_YVU420:
            bpp = 12;
            break;
        case TBM_FORMAT_YUV422:
        case TBM_FORMAT_YVU422:
            bpp = 16;
            break;
        case TBM_FORMAT_YUV444:
        case TBM_FORMAT_YVU444:
            bpp = 24;
            break;
        default :
            break;
    }

    return bpp;
}

static void
_init_surface_bufmgr()
{
    g_surface_bufmgr = tbm_bufmgr_init (-1);
}

static void
_deinit_surface_bufmgr()
{
    if (!g_surface_bufmgr)
        return;

    tbm_bufmgr_deinit (g_surface_bufmgr);
    g_surface_bufmgr = NULL;
}


int
tbm_surface_internal_query_supported_formats (uint32_t **formats, uint32_t *num)
{
    if (!g_surface_bufmgr)
    {
        _init_surface_bufmgr();
        LIST_INITHEAD (&g_surface_list);
    }

    struct _tbm_bufmgr *mgr = g_surface_bufmgr;
    int ret = 0;

    pthread_mutex_lock (&mgr->lock);

    ret = mgr->backend->surface_supported_format (formats, num);

    pthread_mutex_unlock (&mgr->lock);

    return ret;
}

tbm_surface_h
tbm_surface_internal_create_with_flags (int width, int height, int format, int flags)
{
    TBM_RETURN_VAL_IF_FAIL (width > 0, NULL);
    TBM_RETURN_VAL_IF_FAIL (height > 0, NULL);

    if (!g_surface_bufmgr)
    {
        _init_surface_bufmgr();
        LIST_INITHEAD (&g_surface_list);
    }

    struct _tbm_bufmgr *mgr = g_surface_bufmgr;


    TBM_RETURN_VAL_IF_FAIL (TBM_BUFMGR_IS_VALID(mgr), NULL);

    struct _tbm_surface *surf = NULL;
    uint32_t size = 0;
    uint32_t offset = 0;
    uint32_t stride = 0;
    int i;

    surf = calloc (1, sizeof(struct _tbm_surface));
    if (!surf)
        return NULL;

    surf->bufmgr = mgr;
    surf->info.width = width;
    surf->info.height = height;
    surf->info.format = format;
    surf->info.bpp = _tbm_surface_internal_get_bpp (format);
    surf->info.size = tbm_surface_internal_get_size (surf);
    surf->info.num_planes = _tbm_surface_internal_get_num_planes(format);

    /* get size, stride and offset */
    for (i = 0; i < surf->info.num_planes; i++)
    {
        tbm_surface_internal_get_plane_data (surf, i, &size, &offset, &stride);
        surf->info.planes[i].size = size;
        surf->info.planes[i].offset = offset;
        surf->info.planes[i].stride = stride;
    }

    surf->flags = flags;

    /* create only one bo */
    surf->num_bos = 1;
    surf->bos[0] = tbm_bo_alloc (mgr, surf->info.size, flags);
    if (!surf->bos[0])
    {
        free (surf);
        surf = NULL;

        if(LIST_IS_EMPTY (&g_surface_list))
        {
            _deinit_surface_bufmgr ();
            LIST_DELINIT (&g_surface_list);
            return NULL;
        }
    }

    LIST_ADD (&surf->item_link, &g_surface_list);

    return surf;
}

tbm_surface_h
tbm_surface_internal_create_with_bos (int width, int height, int format, tbm_bo *bos, int num)
{
    TBM_RETURN_VAL_IF_FAIL (width > 0, NULL);
    TBM_RETURN_VAL_IF_FAIL (height > 0, NULL);
    TBM_RETURN_VAL_IF_FAIL (bos, NULL);

    if (!g_surface_bufmgr)
    {
        _init_surface_bufmgr();
        LIST_INITHEAD (&g_surface_list);
    }


    struct _tbm_bufmgr *mgr = g_surface_bufmgr;

    TBM_RETURN_VAL_IF_FAIL (TBM_BUFMGR_IS_VALID(mgr), NULL);

    struct _tbm_surface *surf = NULL;
    uint32_t size = 0;
    uint32_t offset = 0;
    uint32_t stride = 0;
    int i;

    surf = calloc (1, sizeof(struct _tbm_surface));
    if (!surf)
        return NULL;

    surf->bufmgr = mgr;
    surf->info.width = width;
    surf->info.height = height;
    surf->info.format = format;
    surf->info.bpp = _tbm_surface_internal_get_bpp (format);
    surf->info.size = tbm_surface_internal_get_size (surf);
    surf->info.num_planes = _tbm_surface_internal_get_num_planes(format);

    /* get size, stride and offset */
    for (i = 0; i < surf->info.num_planes; i++)
    {
        tbm_surface_internal_get_plane_data (surf, i, &size, &offset, &stride);
        surf->info.planes[i].size = size;
        surf->info.planes[i].offset = offset;
        surf->info.planes[i].stride = stride;
    }

    surf->flags = TBM_BO_DEFAULT;

    /* create only one bo */
    surf->num_bos = num;
    for (i = 0; i < num; i++)
    {
        if (bos[i] == NULL)
            goto bail1;

        surf->bos[i] = tbm_bo_ref(bos[i]);
    }

    LIST_ADD (&surf->item_link, &g_surface_list);

    return surf;
bail1:
    for (i = 0; i < num; i++)
    {
        if (surf->bos[i])
        {
            tbm_bo_unref (surf->bos[i]);
            surf->bos[i] = NULL;
        }
    }

    free (surf);
    surf = NULL;

    if(LIST_IS_EMPTY (&g_surface_list))
    {
        _deinit_surface_bufmgr ();
        LIST_DELINIT (&g_surface_list);
    }

    return NULL;
}


void
tbm_surface_internal_destroy (tbm_surface_h surface)
{
    int i;

    if (!surface)
        return;

    surface = (struct _tbm_surface *)surface;

    for (i = 0; i < surface->num_bos; i++)
    {
        tbm_bo_unref (surface->bos[i]);
        surface->bos[i] = NULL;
    }

    LIST_DEL (&surface->item_link);

    free (surface);
    surface = NULL;

    if(LIST_IS_EMPTY (&g_surface_list))
    {
        _deinit_surface_bufmgr ();
        LIST_DELINIT (&g_surface_list);
    }
}


int
tbm_surface_internal_get_num_bos (tbm_surface_h surface)
{
    TBM_RETURN_VAL_IF_FAIL (surface, 0);

    struct _tbm_surface *surf = (struct _tbm_surface *) surface;

    return surf->num_bos;
}

tbm_bo
tbm_surface_internal_get_bo (tbm_surface_h surface, int bo_idx)
{
    TBM_RETURN_VAL_IF_FAIL (surface, NULL);
    TBM_RETURN_VAL_IF_FAIL (bo_idx > -1, NULL);

    struct _tbm_surface *surf = (struct _tbm_surface *) surface;

    return surf->bos[bo_idx];
}

int
tbm_surface_internal_get_size (tbm_surface_h surface)
{
    TBM_RETURN_VAL_IF_FAIL (surface, 0);

    struct _tbm_surface *surf = (struct _tbm_surface *) surface;
    struct _tbm_bufmgr *mgr = surf->bufmgr;
    int size = 0;

    TBM_RETURN_VAL_IF_FAIL (mgr != NULL, 0);
    TBM_RETURN_VAL_IF_FAIL (surf->info.width > 0, 0);
    TBM_RETURN_VAL_IF_FAIL (surf->info.height > 0, 0);
    TBM_RETURN_VAL_IF_FAIL (surf->info.format > 0, 0);

    pthread_mutex_lock (&mgr->lock);

    size = mgr->backend->surface_get_size (surf, surf->info.width, surf->info.height, surf->info.format);

    pthread_mutex_unlock (&mgr->lock);

    return size;
}

int
tbm_surface_internal_get_plane_data (tbm_surface_h surface, int plane_idx, uint32_t *size, uint32_t *offset, uint32_t *pitch)
{
    TBM_RETURN_VAL_IF_FAIL (surface, 0);
    TBM_RETURN_VAL_IF_FAIL (plane_idx > -1, 0);

    struct _tbm_surface *surf = (struct _tbm_surface *) surface;
    struct _tbm_bufmgr *mgr = surf->bufmgr;
    int ret = 0;

    TBM_RETURN_VAL_IF_FAIL (mgr != NULL, 0);
    TBM_RETURN_VAL_IF_FAIL (surf->info.width > 0, 0);
    TBM_RETURN_VAL_IF_FAIL (surf->info.height > 0, 0);
    TBM_RETURN_VAL_IF_FAIL (surf->info.format > 0, 0);

    pthread_mutex_lock (&mgr->lock);

    ret = mgr->backend->surface_get_plane_data (surf, surf->info.width, surf->info.height, surf->info.format, plane_idx, size, offset, pitch);
    if (!ret)
    {
        pthread_mutex_unlock (&mgr->lock);
        return 0;
    }

    pthread_mutex_unlock (&mgr->lock);

    return 1;
}



