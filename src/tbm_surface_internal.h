/**************************************************************************

libtbm

Copyright 2014 Samsung Electronics co., Ltd. All Rights Reserved.

Contact: SooChan Lim <sc1.lim@samsung.com>, Sangjin Lee <lsj119@samsung.com>
Inpyo Kang <mantiger@samsung.com>, Dongyeon Kim <dy5.kim@samsung.com>
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

#ifndef _TBM_SURFACE_INTERNAL_H_
#define _TBM_SURFACE_INTERNAL_H_

#include <tbm_bufmgr.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Queries formats which the system can support.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 * @remarks The formats must be released using free().
 * @param[in] bufmgr : the buffer manager
 * @param[out] *formats : format array which the system can support. This pointer has to be freed by user.
 * @param[out] num : the number of formats.
 * @return a tbm_surface_h if this function succeeds, otherwise NULL
 * @par Example
   @code
   #include <tbm_surface.h>
   #include <tbm_surface_internal.h>

   tbm_bufmgr bufmgr;
   uint32_t *formats;
   uint32_t format_num;

   bufmgr = tbm_bufmgr_create (-1);
   ret = tbm_surface_internal_query_surpported_foramts (bufmgr, &formats, &format_num);

   ...

   free (foramts);
   tbm_surface_bufmgr_deinit (bufmgr);
   @endcode
 */
int tbm_surface_internal_query_supported_formats (uint32_t **formats, uint32_t *num);

/**
 * @brief Creates the tbm_surface with memory type.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 * @details
 * #TBM_BO_DEFAULT is default memory: it depends on the backend\n
 * #TBM_BO_SCANOUT is scanout memory\n
 * #TBM_BO_NONCACHABLE is non-cachable memory\n
 * #TBM_BO_WC is write-combine memory\n
 * #TBM_BO_VENDOR vendor specific memory: it depends on the tbm backend\n
 * @param[in] bufmgr : the buffer manager
 * @param[in] width  : the width of surface
 * @param[in] height : the height of surface
 * @param[in] format : the format of surface
 * @param[in] flags  : the flags of memory type
 * @return a tbm_surface_h if this function succeeds, otherwise NULL
 * @retval #tbm_surface_h
 * @par Example
   @code
   #include <tbm_surface.h>
   #include <tbm_surface_internal.h>

   int bufmgr_fd
   tbm_bufmgr bufmgr;
   tbm_surface_h surface;
   uint32_t *format;
   uint32_t format_num;

   bufmgr = tbm_bufmgr_create (bufmgr_fd);
   surface = tbm_surface_internal_create_with_flags (128, 128, TBM_FORMAT_YUV420, TBM_BO_DEFAULT);

   ...

   tbm_surface_destroy (surface);
   tbm_surface_bufmgr_deinit (bufmgr);
   @endcode
 */
tbm_surface_h tbm_surface_internal_create_with_flags (int width, int height, int format, int flags);

/**
 * @brief Creates the tbm_surface with buffer objects.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 * @param[in] bufmgr : the buffer manager
 * @param[in] width  : the width of surface
 * @param[in] height : the height of surface
 * @param[in] format : the format of surface
 * @param[in] *bos   : the array pointer of buffer objects
 * @param[in] num    : the number of buffer objects
 * @return a tbm_surface_h if this function succeeds, otherwise NULL
 * @retval #tbm_surface_h
 * @par Example
   @code
   #include <tbm_bufmgr.h>
   #include <tbm_surface.h>
   #include <tbm_surface_internal.h>

   int bufmgr_fd
   tbm_bufmgr bufmgr;
   tbm_surface_h surface;
   tbm_surface_info_s info;
   uint32_t *format;
   uint32_t format_num;
   tbm_bo bo[1];

   bufmgr = tbm_bufmgr_init (bufmgr_fd);
   bo[0] = tbm_bo_alloc (bufmgr, 128 * 128, TBM_BO_DEFAULT);

   info.with = 128;
   info.height = 128;
   info.format = TBM_FORMAT_ARGB8888;
   info.bpp = 32;
   info.size = 65536;
   info.num_planes = 1;
   info.planes[0].size = 65536;
   info.planes[0].offset = 0;
   info.planes[0].stride = 512;

   surface = tbm_surface_internal_create_with_bos (&info, bo, 1);

   ...

   tbm_surface_destroy (surface);
   tbm_surface_bufmgr_deinit (bufmgr);
   @endcode
 */
tbm_surface_h tbm_surface_internal_create_with_bos (tbm_surface_info_s *info, tbm_bo *bos, int num);


/**
 * @brief Destroy the tbm surface
    TODO:
 */
void tbm_surface_internal_destroy (tbm_surface_h surface);

/**
 * @brief Gets the number of buffer objects associated with the tbm_surface.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 * @param[in] surface : the tbm_surface_h
 * @return the number of buffer objects associated with the tbm_surface_h, otherwise -1.
 * @par Example
   @code
   #include <tbm_surface.h>
   #include <tbm_surface_internal.h>

   tbm_surface_h surface;
   int num_bos;

   surface = tbm_surface_create (128, 128, TBM_FORMAT_YUV420);
   num_bos = tbm_surface_internal_get_num_bos (surface);

   ...

   tbm_surface_destroy (surface);
   @endcode
 */
int tbm_surface_internal_get_num_bos (tbm_surface_h surface);

/**
 * @brief Gets the buffor object by the bo_index.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 * @param[in] surface : the tbm_surface_h
 * @param[in] bo_idx : the bo index in the the tbm_surface
 * @return the buffer object, otherwise NULL.
 * @retval #tbm_bo
 * @par Example
   @code
   #include <tbm_surface.h>
   #include <tbm_surface_internal.h>

   tbm_surface_h surface;
   int num_bos;
   tbm_bo bo;

   surface = tbm_surface_create (128, 128, TBM_FORMAT_YUV420);
   num_bos = tbm_surface_internal_get_num_bos (surface);

   for (i=0 ; i < num_bos ; i++)
   {
       bo = tbm_surface_internal_get_bo (surface, i);

   ...

   tbm_surface_destroy (surface);
   @endcode
 */
tbm_bo tbm_surface_internal_get_bo (tbm_surface_h surface, int bo_idx);

/**
 * @brief Gets the size of the surface.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 * @param[in] surface : the tbm_surface_h
 * @return the size of tbm_surface, otherwise -1.
 * @par Example
   @code
   #include <tbm_surface.h>
   #include <tbm_surface_internal.h>

   tbm_surface_h surface;
   int size;

   surface = tbm_surface_create (128, 128, TBM_FORMAT_YUV420);
   size = tbm_surface_internal_get_size (surface);

   tbm_surface_destroy (surface);
   @endcode
 */
int tbm_surface_internal_get_size (tbm_surface_h surface);

/**
 * @brief Gets size, offset and pitch data of plane by the plane_index.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 * @param[in] surface : the tbm_surface_h
 * @param[in] plane_idx : the bo index in the the tbm_surface
 * @param[out] size : the size of plan in tbm_surface
 * @param[out] offset : the offset of plan in tbm_surface
 * @param[out] pitch : the pitch of plan in tbm_surface
 * @return 1 if this function succeeds, otherwise 0.
 * @par Example
   @code
   #include <tbm_surface.h>
   #include <tbm_surface_internal.h>

   tbm_surface_h surface;
   uint32_t size, offset, pitch;
   int ret;

   surface = tbm_surfacel_create (128, 128, TBM_FORMAT_YUV420);
   ret = tbm_surface_internal_get_plane_data (surface, 1, &size, &offset, &pitch);

   ...

   tbm_surface_destroy (surface);
   @endcode
 */
int tbm_surface_internal_get_plane_data (tbm_surface_h surface, int plane_idx, uint32_t *size, uint32_t *offset, uint32_t *pitch);

/**
 * @brief Gets number of planes by the format.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 * @param[in] format : the format of surface
 * @return number of planes by the format, otherwise 0.
 * @par Example
   @code
   #include <tbm_surface.h>
   #include <tbm_surface_internal.h>

   int num;

   num = tbm_surface_internal_get_num_planes (TBM_FORMAT_YUV420);

   ...

   @endcode
 */
int tbm_surface_internal_get_num_planes (tbm_format format);

/**
 * @brief Gets bpp by the format.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 * @param[in] format : the format of surface
 * @return bpp by the format, otherwise 0.
 * @par Example
   @code
   #include <tbm_surface.h>
   #include <tbm_surface_internal.h>

   int bpp;

   bpp = tbm_surface_internal_get_bpp (TBM_FORMAT_YUV420);

   ...

   @endcode
 */
int tbm_surface_internal_get_bpp (tbm_format format);

#ifdef __cplusplus
}
#endif

#endif /* _TBM_SURFACE_INTERNAL_H_ */

