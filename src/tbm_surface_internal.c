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

static pthread_mutex_t tbm_surface_lock;

static bool
_tbm_surface_mutex_init (void)
{
    static bool tbm_surface_mutex_init = false;

    if (tbm_surface_mutex_init)
        return true;

    if (pthread_mutex_init (&tbm_surface_lock, NULL))
    {
        TBM_LOG ("[libtbm] fail: tbm_surface mutex init\n");
        return false;
    }

    tbm_surface_mutex_init = true;

    return true;
}

void
_tbm_surface_mutex_lock (void)
{
    if (!_tbm_surface_mutex_init ())
        return;

    pthread_mutex_lock (&tbm_surface_lock);
}

void
_tbm_surface_mutex_unlock (void)
{
    pthread_mutex_unlock (&tbm_surface_lock);
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

static int
_tbm_surface_internal_query_size (tbm_surface_h surface)
{
    TBM_RETURN_VAL_IF_FAIL (surface, 0);

    struct _tbm_surface *surf = (struct _tbm_surface *) surface;
    struct _tbm_bufmgr *mgr = surf->bufmgr;
    int size = 0;

    TBM_RETURN_VAL_IF_FAIL (mgr != NULL, 0);
    TBM_RETURN_VAL_IF_FAIL (surf->info.width > 0, 0);
    TBM_RETURN_VAL_IF_FAIL (surf->info.height > 0, 0);
    TBM_RETURN_VAL_IF_FAIL (surf->info.format > 0, 0);

    if (!mgr->backend->surface_get_size)
        return 0;

    size = mgr->backend->surface_get_size (surf, surf->info.width, surf->info.height, surf->info.format);

    return size;
}

static int
_tbm_surface_internal_query_plane_data (tbm_surface_h surface, int plane_idx, uint32_t *size, uint32_t *offset, uint32_t *pitch, int *bo_idx)
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

    if (!mgr->backend->surface_get_plane_data)
        return 0;

    ret = mgr->backend->surface_get_plane_data (surf, surf->info.width, surf->info.height, surf->info.format, plane_idx, size, offset, pitch, bo_idx);
    if (!ret)
        return 0;

    return 1;
}

static int
_tbm_surface_internal_query_num_bos (tbm_format format)
{
    TBM_RETURN_VAL_IF_FAIL (format > 0, 0);
    struct _tbm_bufmgr *mgr;
    int ret = 0;

    mgr = g_surface_bufmgr;

    if (!mgr->backend->surface_get_num_bos)
        return 0;

    ret = mgr->backend->surface_get_num_bos (format);
    if (!ret)
        return 0;

    return ret;
}

static void
_tbm_surface_internal_destroy (tbm_surface_h surface)
{
    int i;

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
tbm_surface_internal_query_supported_formats (uint32_t **formats, uint32_t *num)
{
    struct _tbm_bufmgr *mgr;
    int ret = 0;

    _tbm_surface_mutex_lock();

    if (!g_surface_bufmgr)
    {
        _init_surface_bufmgr();
        LIST_INITHEAD (&g_surface_list);
    }

    mgr = g_surface_bufmgr;

    if (!mgr->backend->surface_supported_format)
    {
        _tbm_surface_mutex_unlock();
        return 0;
    }

    ret = mgr->backend->surface_supported_format (formats, num);

    _tbm_surface_mutex_unlock();

    return ret;
}


int tbm_surface_internal_get_num_planes (tbm_format format)
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

int tbm_surface_internal_get_bpp (tbm_format format)
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

tbm_surface_h
tbm_surface_internal_create_with_flags (int width, int height, int format, int flags)
{
    TBM_RETURN_VAL_IF_FAIL (width > 0, NULL);
    TBM_RETURN_VAL_IF_FAIL (height > 0, NULL);

    struct _tbm_bufmgr *mgr;
    struct _tbm_surface *surf = NULL;
    uint32_t size = 0;
    uint32_t offset = 0;
    uint32_t stride = 0;
    uint32_t bo_size = 0;
    int bo_idx;
    int i, j;

    _tbm_surface_mutex_lock();

    if (!g_surface_bufmgr)
    {
        _init_surface_bufmgr();
        LIST_INITHEAD (&g_surface_list);
    }

    mgr = g_surface_bufmgr;
    if (!TBM_BUFMGR_IS_VALID(mgr))
    {
        _tbm_surface_mutex_unlock();
        return NULL;
    }
    surf = calloc (1, sizeof(struct _tbm_surface));
    if (!surf)
    {
        _tbm_surface_mutex_unlock();
        return NULL;
    }

    surf->bufmgr = mgr;
    surf->info.width = width;
    surf->info.height = height;
    surf->info.format = format;
    surf->info.bpp = tbm_surface_internal_get_bpp (format);
    surf->info.size = _tbm_surface_internal_query_size (surf);
    surf->info.num_planes = tbm_surface_internal_get_num_planes(format);
    surf->num_bos = _tbm_surface_internal_query_num_bos(format);
    surf->refcnt = 1;

    /* get size, stride and offset bo_idx*/
    for (i = 0; i < surf->info.num_planes; i++)
    {
        _tbm_surface_internal_query_plane_data (surf, i, &size, &offset, &stride, &bo_idx);
        surf->info.planes[i].size = size;
        surf->info.planes[i].offset = offset;
        surf->info.planes[i].stride = stride;
        surf->planes_bo_idx[i] = bo_idx;
    }

    surf->flags = flags;

    for (i = 0; i < surf->num_bos; i++)
    {
        bo_size = 0;
        for (j = 0; j < surf->info.num_planes; j++)
        {
            if (surf->planes_bo_idx[i] == i)
                bo_size += surf->info.planes[i].size;
        }

        surf->bos[i] = tbm_bo_alloc (mgr, bo_size, flags);
        if (!surf->bos[i]) {
            for (j = 0; j < i; j++)
                tbm_bo_unref (surf->bos[j]);

            free (surf);
            surf = NULL;

            if(LIST_IS_EMPTY (&g_surface_list))
            {
                _deinit_surface_bufmgr ();
                LIST_DELINIT (&g_surface_list);
            }

            _tbm_surface_mutex_unlock();
            return NULL;
        }
    }

    LIST_ADD (&surf->item_link, &g_surface_list);

    _tbm_surface_mutex_unlock();

    return surf;
}

tbm_surface_h
tbm_surface_internal_create_with_bos (tbm_surface_info_s *info, tbm_bo *bos, int num)
{
    TBM_RETURN_VAL_IF_FAIL (bos, NULL);
    TBM_RETURN_VAL_IF_FAIL (info, NULL);
    TBM_RETURN_VAL_IF_FAIL (num == 1 || info->num_planes == num, NULL);

    struct _tbm_bufmgr *mgr;
    struct _tbm_surface *surf = NULL;
    int i;

    _tbm_surface_mutex_lock();

    if (!g_surface_bufmgr)
    {
        _init_surface_bufmgr();
        LIST_INITHEAD (&g_surface_list);
    }

    mgr = g_surface_bufmgr;
    if (!TBM_BUFMGR_IS_VALID(mgr))
    {
        _tbm_surface_mutex_unlock();
        return NULL;
    }

    surf = calloc (1, sizeof(struct _tbm_surface));
    if (!surf)
    {
        _tbm_surface_mutex_unlock();
        return NULL;
    }

    surf->bufmgr = mgr;
    surf->info.width = info->width;
    surf->info.height = info->height;
    surf->info.format = info->format;
    surf->info.bpp = info->bpp;
    surf->info.num_planes = info->num_planes;
    surf->refcnt = 1;

    /* get size, stride and offset */
    for (i = 0; i < info->num_planes; i++)
    {
        surf->info.planes[i].offset = info->planes[i].offset;
        surf->info.planes[i].stride = info->planes[i].stride;

        if (info->planes[i].size > 0)
            surf->info.planes[i].size = info->planes[i].size;
        else
            surf->info.planes[i].size += surf->info.planes[i].stride * info->height;

        if (num == 1)
            surf->planes_bo_idx[i] = 0;
        else
            surf->planes_bo_idx[i] = i;
    }

    if (info->size > 0)
    {
        surf->info.size = info->size;
    }
    else
    {
        surf->info.size = 0;
        for (i = 0; i < info->num_planes; i++)
        {
            surf->info.size += surf->info.planes[i].size;
        }
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

    _tbm_surface_mutex_unlock();

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

    _tbm_surface_mutex_unlock();

    return NULL;
}


void
tbm_surface_internal_destroy (tbm_surface_h surface)
{
    if (!surface)
        return;

    _tbm_surface_mutex_lock();

    surface->refcnt--;

    if (surface->refcnt > 0) {
        _tbm_surface_mutex_unlock();
        return;
    }

    if (surface->refcnt == 0)
        _tbm_surface_internal_destroy(surface);

    _tbm_surface_mutex_unlock();
}


void
tbm_surface_internal_ref (tbm_surface_h surface)
{
    TBM_RETURN_IF_FAIL (surface);

    _tbm_surface_mutex_lock();

    surface->refcnt++;

    _tbm_surface_mutex_unlock();
}

void
tbm_surface_internal_unref (tbm_surface_h surface)
{
    TBM_RETURN_IF_FAIL (surface);

    _tbm_surface_mutex_lock();

    surface->refcnt--;

    if (surface->refcnt > 0) {
        _tbm_surface_mutex_unlock();
        return;
    }

    if (surface->refcnt == 0)
        _tbm_surface_internal_destroy(surface);

    _tbm_surface_mutex_unlock();
}

int
tbm_surface_internal_get_num_bos (tbm_surface_h surface)
{
    TBM_RETURN_VAL_IF_FAIL (surface, 0);

    struct _tbm_surface *surf;
    int num;

    _tbm_surface_mutex_lock();

    surf = (struct _tbm_surface *) surface;
    num = surf->num_bos;

    _tbm_surface_mutex_unlock();

    return num;
}

tbm_bo
tbm_surface_internal_get_bo (tbm_surface_h surface, int bo_idx)
{
    TBM_RETURN_VAL_IF_FAIL (surface, NULL);
    TBM_RETURN_VAL_IF_FAIL (bo_idx > -1, NULL);

    struct _tbm_surface *surf;
    tbm_bo bo;

    _tbm_surface_mutex_lock();

    surf = (struct _tbm_surface *) surface;
    bo = surf->bos[bo_idx];

    _tbm_surface_mutex_unlock();

    return bo;
}

int
tbm_surface_internal_get_size (tbm_surface_h surface)
{
    TBM_RETURN_VAL_IF_FAIL (surface, 0);

    struct _tbm_surface *surf;
    unsigned int size;

    _tbm_surface_mutex_lock();

    surf = (struct _tbm_surface *) surface;
    size = surf->info.size;

    _tbm_surface_mutex_unlock();

    return size;
}

int
tbm_surface_internal_get_plane_data (tbm_surface_h surface, int plane_idx, uint32_t *size, uint32_t *offset, uint32_t *pitch)
{
    TBM_RETURN_VAL_IF_FAIL (surface, 0);
    TBM_RETURN_VAL_IF_FAIL (plane_idx > -1, 0);

    struct _tbm_surface *surf;

    _tbm_surface_mutex_lock();

    surf = (struct _tbm_surface *) surface;

    if (plane_idx >= surf->info.num_planes)
    {
        _tbm_surface_mutex_unlock();
        return 0;
    }

    *size = surf->info.planes[plane_idx].size;
    *offset = surf->info.planes[plane_idx].offset;
    *pitch = surf->info.planes[plane_idx].stride;

    _tbm_surface_mutex_unlock();

    return 1;
}

int
tbm_surface_internal_get_info (tbm_surface_h surface, int opt, tbm_surface_info_s *info, int map)
{
    struct _tbm_surface *surf;
    tbm_bo_handle bo_handles[4];
    int i, j;

    _tbm_surface_mutex_lock();

    memset (bo_handles, 0, sizeof(tbm_bo_handle) * 4);

    surf = (struct _tbm_surface *)surface;

    info->width = surf->info.width;
    info->height = surf->info.height;
    info->format = surf->info.format;
    info->bpp = surf->info.bpp;
    info->size = surf->info.size;
    info->num_planes = surf->info.num_planes;

    if (map == 1)
    {
        for (i = 0; i < surf->num_bos; i++)
        {
            bo_handles[i] = tbm_bo_map (surf->bos[i], TBM_DEVICE_CPU, opt);
            if (bo_handles[i].ptr == NULL)
            {
                for (j = 0; j < i; j++)
                    tbm_bo_unmap (surf->bos[j]);

                _tbm_surface_mutex_unlock();
                return 0;
            }
        }
    }
    else
    {
        for (i = 0; i < surf->num_bos; i++)
        {
            bo_handles[i] = tbm_bo_get_handle (surf->bos[i], TBM_DEVICE_CPU);
            if (bo_handles[i].ptr == NULL)
            {
                _tbm_surface_mutex_unlock();
                return 0;
            }
        }
    }

    for (i = 0; i < surf->info.num_planes; i++)
    {
        info->planes[i].size = surf->info.planes[i].size;
        info->planes[i].offset = surf->info.planes[i].offset;
        info->planes[i].stride = surf->info.planes[i].stride;
        info->planes[i].ptr = bo_handles[surf->planes_bo_idx[i]].ptr + surf->info.planes[i].offset;
    }

    _tbm_surface_mutex_unlock();

    return 1;
}

void
tbm_surface_internal_unmap (tbm_surface_h surface)
{
    struct _tbm_surface *surf;
    int i;

    _tbm_surface_mutex_lock();

    surf = (struct _tbm_surface *)surface;

    for (i = 0; i < surf->num_bos; i++)
        tbm_bo_unmap (surf->bos[i]);

    _tbm_surface_mutex_unlock();
}

unsigned int
tbm_surface_internal_get_width (tbm_surface_h surface)
{
    struct _tbm_surface *surf;
    unsigned int width;

    _tbm_surface_mutex_lock();

    surf = (struct _tbm_surface *)surface;
    width = surf->info.width;

    _tbm_surface_mutex_unlock();

    return width;
}

unsigned int
tbm_surface_internal_get_height (tbm_surface_h surface)
{
    struct _tbm_surface *surf;
    unsigned int height;

    _tbm_surface_mutex_lock();

    surf = (struct _tbm_surface *)surface;
    height = surf->info.height;

    _tbm_surface_mutex_unlock();

    return height;

}

tbm_format
tbm_surface_internal_get_format (tbm_surface_h surface)
{
    struct _tbm_surface *surf;
    tbm_format format;

    _tbm_surface_mutex_lock();

    surf = (struct _tbm_surface *)surface;
    format = surf->info.format;

    _tbm_surface_mutex_unlock();

    return format;
}

int
tbm_surface_internal_get_plane_bo_idx (tbm_surface_h surface, int plane_idx)
{
    TBM_RETURN_VAL_IF_FAIL (surface, 0);
    TBM_RETURN_VAL_IF_FAIL (plane_idx > -1, 0);
    struct _tbm_surface *surf;
    int bo_idx;

    _tbm_surface_mutex_lock();

    surf = (struct _tbm_surface *)surface;
    bo_idx = surf->planes_bo_idx[plane_idx];

    _tbm_surface_mutex_unlock();

    return bo_idx;
}

