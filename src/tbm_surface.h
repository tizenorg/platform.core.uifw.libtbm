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

#ifndef _TBM_SURFACE_H_
#define _TBM_SURFACE_H_

/**
 * @addtogroup CAPI_UI_TBM_SURFACE_MODULE
 * @{
 */

#include <tbm_type.h>
#include <tizen.h>

/**
 * \file tbm_surface.h
 * \brief TBM Surface
 */

/**
 * @brief Enumeration of tbm_surface error type
 * @since_tizen 2.3
 */
typedef enum
{
    TBM_SURFACE_ERROR_NONE  = TIZEN_ERROR_NONE, /**< Successful */
    TBM_SURFACE_ERROR_INVALID_PARAMETER  = TIZEN_ERROR_INVALID_PARAMETER, /**< Invalid parameter */
    TBM_SURFACE_ERROR_INVALID_OPERATION  = TIZEN_ERROR_INVALID_OPERATION, /**< Invalid Operation */
} tbm_surface_error_e;

/**
 * @brief Definition for the max number of tbm surface plane.
 * @since_tizen 2.3
 */
#define TBM_SURF_PLANE_MAX 4

/* option to map the tbm_surface */
/**
 * @brief Definition for the access option to read.
 * @since_tizen 2.3
 */
#define TBM_SURF_OPTION_READ      (1 << 0)
/**
 * @brief Definition for the access option to write.
 * @since_tizen 2.3
 */
#define TBM_SURF_OPTION_WRITE     (1 << 1)

/**
 * @brief Definition for the tbm surface infomation struct.
 * @since_tizen 2.3
 */
typedef struct _tbm_surface_info
{
    uint32_t width; /**< tbm surface width */
    uint32_t height; /**< tbm surface height */
    tbm_format format; /**<  tbm surface foramt*/
    uint32_t bpp; /**< tbm surface bbp */
    uint32_t size; /**< tbm surface size */

    uint32_t num_planes; /**< the number of planes */

    struct {
        unsigned char *ptr; /**< plane pointer */
        uint32_t size; /**< plane size */
        uint32_t offset; /**< plane offset */
        uint32_t stride; /**< plane stride */

        void *reserved1;
        void *reserved2;
        void *reserved3;
    } planes[TBM_SURF_PLANE_MAX]; /**< array of planes */

    void *reserved4;
    void *reserved5;
    void *reserved6;
} tbm_surface_info_s;

#define __tbm_fourcc_code(a,b,c,d) ((uint32_t)(a) | ((uint32_t)(b) << 8) | \
			      ((uint32_t)(c) << 16) | ((uint32_t)(d) << 24))

/* color index */
/**
 * @brief Definition for the tbm surface foramt C8 ([7:0] C)
 * @since_tizen 2.3
 */
#define TBM_FORMAT_C8		__tbm_fourcc_code('C', '8', ' ', ' ')

/* 8 bpp RGB */
/**
 * @brief Definition for the tbm surface foramt RGB322  ([7:0] R:G:B 3:3:2)
 * @since_tizen 2.3
 */
#define TBM_FORMAT_RGB332	__tbm_fourcc_code('R', 'G', 'B', '8')
/**
 * @brief Definition for the tbm surface foramt RGB233  ([7:0] B:G:R 2:3:3)
 * @since_tizen 2.3
 */
#define TBM_FORMAT_BGR233	__tbm_fourcc_code('B', 'G', 'R', '8')

/* 16 bpp RGB */
/**
 * @brief Definition for the tbm surface foramt XRGB4444 ([15:0] x:R:G:B 4:4:4:4 little endian)
 * @since_tizen 2.3
 */
#define TBM_FORMAT_XRGB4444	__tbm_fourcc_code('X', 'R', '1', '2')
/**
 * @brief Definition for the tbm surface foramt XBRG4444 ([15:0] x:B:G:R 4:4:4:4 little endian)
 * @since_tizen 2.3
 */
#define TBM_FORMAT_XBGR4444	__tbm_fourcc_code('X', 'B', '1', '2')
/**
 * @brief Definition for the tbm surface foramt RGBX4444 ([15:0] R:G:B:x 4:4:4:4 little endian)
 * @since_tizen 2.3
 */
#define TBM_FORMAT_RGBX4444	__tbm_fourcc_code('R', 'X', '1', '2')
/**
 * @brief Definition for the tbm surface foramt BGRX4444 ([15:0] B:G:R:x 4:4:4:4 little endian)
 * @since_tizen 2.3
 */
#define TBM_FORMAT_BGRX4444	__tbm_fourcc_code('B', 'X', '1', '2')

/**
 * @brief Definition for the tbm surface foramt ARGB4444 ([15:0] A:R:G:B 4:4:4:4 little endian)
 * @since_tizen 2.3
 */
#define TBM_FORMAT_ARGB4444	__tbm_fourcc_code('A', 'R', '1', '2')
/**
 * @brief Definition for the tbm surface foramt ABGR4444 ([15:0] A:B:G:R 4:4:4:4 little endian)
 * @since_tizen 2.3
 */
#define TBM_FORMAT_ABGR4444	__tbm_fourcc_code('A', 'B', '1', '2')
/**
 * @brief Definition for the tbm surface foramt RGBA4444 ([15:0] R:G:B:A 4:4:4:4 little endian)
 * @since_tizen 2.3
 */
#define TBM_FORMAT_RGBA4444	__tbm_fourcc_code('R', 'A', '1', '2')
/**
 * @brief Definition for the tbm surface foramt BGRA4444 ([15:0] B:G:R:A 4:4:4:4 little endian)
 * @since_tizen 2.3
 */
#define TBM_FORMAT_BGRA4444	__tbm_fourcc_code('B', 'A', '1', '2')

/**
 * @brief Definition for the tbm surface foramt XRGB1555 ([15:0] x:R:G:B 1:5:5:5 little endian)
 * @since_tizen 2.3
 */
#define TBM_FORMAT_XRGB1555	__tbm_fourcc_code('X', 'R', '1', '5')
/**
 * @brief Definition for the tbm surface foramt XBGR1555 ([15:0] x:B:G:R 1:5:5:5 little endian)
 * @since_tizen 2.3
 */
#define TBM_FORMAT_XBGR1555	__tbm_fourcc_code('X', 'B', '1', '5')
/**
 * @brief Definition for the tbm surface foramt RGBX5551 ([15:0] R:G:B:x 5:5:5:1 little endian)
 * @since_tizen 2.3
 */
#define TBM_FORMAT_RGBX5551	__tbm_fourcc_code('R', 'X', '1', '5')
/**
 * @brief Definition for the tbm surface foramt BGRX5551 ([15:0] B:G:R:x 5:5:5:1 little endian)
 * @since_tizen 2.3
 */
#define TBM_FORMAT_BGRX5551	__tbm_fourcc_code('B', 'X', '1', '5')

/**
 * @brief Definition for the tbm surface foramt ARGB1555 ([15:0] A:R:G:B 1:5:5:5 little endian)
 * @since_tizen 2.3
 */
#define TBM_FORMAT_ARGB1555	__tbm_fourcc_code('A', 'R', '1', '5')
/**
 * @brief Definition for the tbm surface foramt ABGR1555 ([15:0] A:B:G:R 1:5:5:5 little endian)
 * @since_tizen 2.3
 */
#define TBM_FORMAT_ABGR1555	__tbm_fourcc_code('A', 'B', '1', '5')
/**
 * @brief Definition for the tbm surface foramt RGBA5551 ([15:0] R:G:B:A 5:5:5:1 little endian)
 * @since_tizen 2.3
 */
#define TBM_FORMAT_RGBA5551	__tbm_fourcc_code('R', 'A', '1', '5')
/**
 * @brief Definition for the tbm surface foramt BGRA5551 ([15:0] B:G:R:A 5:5:5:1 little endian)
 * @since_tizen 2.3
 */
#define TBM_FORMAT_BGRA5551	__tbm_fourcc_code('B', 'A', '1', '5')

/**
 * @brief Definition for the tbm surface foramt RGB565 ([15:0] R:G:B 5:6:5 little endian)
 * @since_tizen 2.3
 */
#define TBM_FORMAT_RGB565	__tbm_fourcc_code('R', 'G', '1', '6')
/**
 * @brief Definition for the tbm surface foramt BGR565 ([15:0] B:G:R 5:6:5 little endian)
 * @since_tizen 2.3
 */
#define TBM_FORMAT_BGR565	__tbm_fourcc_code('B', 'G', '1', '6')

/* 24 bpp RGB */
/**
 * @brief Definition for the tbm surface foramt RGB888 ([23:0] R:G:B little endian)
 * @since_tizen 2.3
 */
#define TBM_FORMAT_RGB888	__tbm_fourcc_code('R', 'G', '2', '4')
/**
 * @brief Definition for the tbm surface foramt BGR888 ([23:0] B:G:R little endian)
 * @since_tizen 2.3
 */
#define TBM_FORMAT_BGR888	__tbm_fourcc_code('B', 'G', '2', '4')

/* 32 bpp RGB */
/**
 * @brief Definition for the tbm surface foramt XRGB8888 ([31:0] x:R:G:B 8:8:8:8 little endian)
 * @since_tizen 2.3
 */
#define TBM_FORMAT_XRGB8888	__tbm_fourcc_code('X', 'R', '2', '4')
/**
 * @brief Definition for the tbm surface foramt XBGR8888 ([31:0] x:B:G:R 8:8:8:8 little endian)
 * @since_tizen 2.3
 */
#define TBM_FORMAT_XBGR8888	__tbm_fourcc_code('X', 'B', '2', '4')
/**
 * @brief Definition for the tbm surface foramt RGBX8888 ([31:0] R:G:B:x 8:8:8:8 little endian)
 * @since_tizen 2.3
 */
#define TBM_FORMAT_RGBX8888	__tbm_fourcc_code('R', 'X', '2', '4')
/**
 * @brief Definition for the tbm surface foramt BGRX8888 ([31:0] B:G:R:x 8:8:8:8 little endian)
 * @since_tizen 2.3
 */
#define TBM_FORMAT_BGRX8888	__tbm_fourcc_code('B', 'X', '2', '4')

/**
 * @brief Definition for the tbm surface foramt ARGB8888 ([31:0] A:R:G:B 8:8:8:8 little endian)
 * @since_tizen 2.3
 */
#define TBM_FORMAT_ARGB8888	__tbm_fourcc_code('A', 'R', '2', '4')
/**
 * @brief Definition for the tbm surface foramt ABGR8888 ([31:0] [31:0] A:B:G:R 8:8:8:8 little endian)
 * @since_tizen 2.3
 */
#define TBM_FORMAT_ABGR8888	__tbm_fourcc_code('A', 'B', '2', '4')
/**
 * @brief Definition for the tbm surface foramt RGBA8888 ([31:0] R:G:B:A 8:8:8:8 little endian)
 * @since_tizen 2.3
 */
#define TBM_FORMAT_RGBA8888	__tbm_fourcc_code('R', 'A', '2', '4')
/**
 * @brief Definition for the tbm surface foramt BGRA8888 ([31:0] B:G:R:A 8:8:8:8 little endian)
 * @since_tizen 2.3
 */
#define TBM_FORMAT_BGRA8888	__tbm_fourcc_code('B', 'A', '2', '4')

/**
 * @brief Definition for the tbm surface foramt XRGB2101010 ([31:0] x:R:G:B 2:10:10:10 little endian)
 * @since_tizen 2.3
 */
#define TBM_FORMAT_XRGB2101010	__tbm_fourcc_code('X', 'R', '3', '0')
/**
 * @brief Definition for the tbm surface foramt XBGR2101010 ([31:0] x:B:G:R 2:10:10:10 little endian)
 * @since_tizen 2.3
 */
#define TBM_FORMAT_XBGR2101010	__tbm_fourcc_code('X', 'B', '3', '0')
/**
 * @brief Definition for the tbm surface foramt RGBX1010102 ([31:0] R:G:B:x 10:10:10:2 little endian)
 * @since_tizen 2.3
 */
#define TBM_FORMAT_RGBX1010102	__tbm_fourcc_code('R', 'X', '3', '0')
/**
 * @brief Definition for the tbm surface foramt BGRX1010102 ([31:0] B:G:R:x 10:10:10:2 little endian)
 * @since_tizen 2.3
 */
#define TBM_FORMAT_BGRX1010102	__tbm_fourcc_code('B', 'X', '3', '0')

/**
 * @brief Definition for the tbm surface foramt ARGB2101010 ([31:0] A:R:G:B 2:10:10:10 little endian)
 * @since_tizen 2.3
 */
#define TBM_FORMAT_ARGB2101010	__tbm_fourcc_code('A', 'R', '3', '0')
/**
 * @brief Definition for the tbm surface foramt ABGR2101010 ([31:0] A:B:G:R 2:10:10:10 little endian)
 * @since_tizen 2.3
 */
#define TBM_FORMAT_ABGR2101010	__tbm_fourcc_code('A', 'B', '3', '0')
/**
 * @brief Definition for the tbm surface foramt RGBA1010102 ([31:0] R:G:B:A 10:10:10:2 little endian)
 * @since_tizen 2.3
 */
#define TBM_FORMAT_RGBA1010102	__tbm_fourcc_code('R', 'A', '3', '0')
/**
 * @brief Definition for the tbm surface foramt BGRA1010102 ([31:0] B:G:R:A 10:10:10:2 little endian)
 * @since_tizen 2.3
 */
#define TBM_FORMAT_BGRA1010102	__tbm_fourcc_code('B', 'A', '3', '0') /*  */

/* packed YCbCr */
/**
 * @brief Definition for the tbm surface foramt YUYV ([31:0] Cr0:Y1:Cb0:Y0 8:8:8:8 little endian)
 * @since_tizen 2.3
 */
#define TBM_FORMAT_YUYV		__tbm_fourcc_code('Y', 'U', 'Y', 'V')
/**
 * @brief Definition for the tbm surface foramt YVYU ([31:0] Cb0:Y1:Cr0:Y0 8:8:8:8 little endian)
 * @since_tizen 2.3
 */
#define TBM_FORMAT_YVYU		__tbm_fourcc_code('Y', 'V', 'Y', 'U') /*  */
/**
 * @brief Definition for the tbm surface foramt UYVY ([31:0] Y1:Cr0:Y0:Cb0 8:8:8:8 little endian )
 * @since_tizen 2.3
 */
#define TBM_FORMAT_UYVY		__tbm_fourcc_code('U', 'Y', 'V', 'Y')
/**
 * @brief Definition for the tbm surface foramt VYUY ([31:0] Y1:Cb0:Y0:Cr0 8:8:8:8 little endian)
 * @since_tizen 2.3
 */
#define TBM_FORMAT_VYUY		__tbm_fourcc_code('V', 'Y', 'U', 'Y')

/**
 * @brief Definition for the tbm surface foramt AYUV ([31:0] A:Y:Cb:Cr 8:8:8:8 little endian)
 * @since_tizen 2.3
 */
#define TBM_FORMAT_AYUV		__tbm_fourcc_code('A', 'Y', 'U', 'V')

/*
 * 2 plane YCbCr
 * index 0 = Y plane, [7:0] Y
 * index 1 = Cr:Cb plane, [15:0] Cr:Cb little endian
 * or
 * index 1 = Cb:Cr plane, [15:0] Cb:Cr little endian
 */
/**
 * @brief Definition for the tbm surface foramt NV12 (2x2 subsampled Cr:Cb plane)
 * @since_tizen 2.3
 */
#define TBM_FORMAT_NV12		__tbm_fourcc_code('N', 'V', '1', '2')
/**
 * @brief Definition for the tbm surface foramt NV21 (2x2 subsampled Cb:Cr plane)
 * @since_tizen 2.3
 */
#define TBM_FORMAT_NV21		__tbm_fourcc_code('N', 'V', '2', '1') /*  */
/**
 * @brief Definition for the tbm surface foramt NV16 (2x1 subsampled Cr:Cb plane)
 * @since_tizen 2.3
 */
#define TBM_FORMAT_NV16		__tbm_fourcc_code('N', 'V', '1', '6')
/**
 * @brief Definition for the tbm surface foramt NV61 (2x1 subsampled Cb:Cr plane)
 * @since_tizen 2.3
 */
#define TBM_FORMAT_NV61		__tbm_fourcc_code('N', 'V', '6', '1')

/*
 * 3 plane YCbCr
 * index 0: Y plane, [7:0] Y
 * index 1: Cb plane, [7:0] Cb
 * index 2: Cr plane, [7:0] Cr
 * or
 * index 1: Cr plane, [7:0] Cr
 * index 2: Cb plane, [7:0] Cb
 */
/**
 * @brief Definition for the tbm surface foramt YUV410 (4x4 subsampled Cb (1) and Cr (2) planes)
 */
#define TBM_FORMAT_YUV410	__tbm_fourcc_code('Y', 'U', 'V', '9')
/**
 * @brief Definition for the tbm surface foramt YVU410 (4x4 subsampled Cr (1) and Cb (2) planes)
 * @since_tizen 2.3
 */
#define TBM_FORMAT_YVU410	__tbm_fourcc_code('Y', 'V', 'U', '9')
/**
 * @brief Definition for the tbm surface foramt YUV411 (4x1 subsampled Cb (1) and Cr (2) planes)
 * @since_tizen 2.3
 */
#define TBM_FORMAT_YUV411	__tbm_fourcc_code('Y', 'U', '1', '1')
/**
 * @brief Definition for the tbm surface foramt YVU411 (4x1 subsampled Cr (1) and Cb (2) planes)
 * @since_tizen 2.3
 */
#define TBM_FORMAT_YVU411	__tbm_fourcc_code('Y', 'V', '1', '1')
/**
 * @brief Definition for the tbm surface foramt YUV420 (2x2 subsampled Cb (1) and Cr (2) planes)
 * @since_tizen 2.3
 */
#define TBM_FORMAT_YUV420	__tbm_fourcc_code('Y', 'U', '1', '2')
/**
 * @brief Definition for the tbm surface foramt YVU420 (2x2 subsampled Cr (1) and Cb (2) planes)
 * @since_tizen 2.3
 */
#define TBM_FORMAT_YVU420	__tbm_fourcc_code('Y', 'V', '1', '2')
/**
 * @brief Definition for the tbm surface foramt YUV422 (2x1 subsampled Cb (1) and Cr (2) planes)
 * @since_tizen 2.3
 */
#define TBM_FORMAT_YUV422	__tbm_fourcc_code('Y', 'U', '1', '6')
/**
 * @brief Definition for the tbm surface foramt YVU422 (2x1 subsampled Cr (1) and Cb (2) planes)
 * @since_tizen 2.3
 */
#define TBM_FORMAT_YVU422	__tbm_fourcc_code('Y', 'V', '1', '6')
/**
 * @brief Definition for the tbm surface foramt YUV444 (non-subsampled Cb (1) and Cr (2) planes)
 * @since_tizen 2.3
 */
#define TBM_FORMAT_YUV444	__tbm_fourcc_code('Y', 'U', '2', '4')
/**
 * @brief Definition for the tbm surface foramt YVU444 (non-subsampled Cr (1) and Cb (2) planes)
 * @since_tizen 2.3
 */
#define TBM_FORMAT_YVU444	__tbm_fourcc_code('Y', 'V', '2', '4')

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Queries surface format list and number of format supported by the system.
 * @since_tizen 2.3
 * @remarks The formats must be released using free().
 * @param[out] **formats : format array which the system can support. This pointer has to be freed by user.
 * @param[out] num : the number of formats.
 * @return #TBM_SURFACE_ERROR_NONE if this function succeeds, otherwise error status value
 * @retval #TBM_SURFACE_ERROR_NONE
 * @retval #TBM_SURFACE_ERROR_INVALID_OPERATION
 * @par Example
   @code
   #include <tbm_surface.h>

   uint32_t *formats;
   uint32_t format_num;
   int ret, i;

   if (tbm_surface_query_formats (&formats, &format_num))
   {
       for( i = 0 ; i < format_num ; i++)
       {
           if (formats[i] == TBM_FORMAT_RGB332)
           {

   ....

   free (formats);
   @endcode
 */
int tbm_surface_query_formats (uint32_t **formats, uint32_t *num);

/**
 * @brief Creates the tbm_surface.
 * @details This function create the tbm_surface with width, height, format.
 * @since_tizen 2.3
 * @remark The specific error code can be obtained using the get_last_result() method. Error codes are described in Exception section.
 * @param[in] width  : the width of surface
 * @param[in] height : the height of surface
 * @param[in] format : the format of surface
 * @return a tbm_surface_h if this function succeeds, otherwise NULL
 * @retval #tbm_surface_h
 * @exception #TBM_SURFACE_ERROR_NONE Success
 * @exception #TBM_SURFACE_ERROR_INVALID_PARAMETER  Invalid parameter
 * @exception #TBM_SURFACE_ERROR_INVALID_OPERATION Invalid operation
 * @see tbm_surface_destroy()
 * @par Example
   @code
   #include <tbm_surface.h>

   tbm_surface_h surface;

   surface = tbm_surface_create (128, 128, TBM_FORMAT_RGB332);

   ...

   tbm_surface_destroy (surface);
   @endcode
 */
tbm_surface_h tbm_surface_create (int width, int height, tbm_format format);

/**
 * @brief Destroies the tbm_surface.
 * @since_tizen 2.3
 * @param[in] surface : the tbm_surface_h
 * @return #TBM_SURFACE_ERROR_NONE if this function succeeds, otherwise error status value
 * @retval #TBM_SURFACE_ERROR_NONE
 * @retval #TBM_SURFACE_ERROR_INVALID_PARAMETER
 * @see tbm_surface_create()
 * @par Example
   @code
   #include <tbm_surface.h>

   tbm_surface_h surface;

   surface = tbm_surface_create (128, 128, TBM_FORMAT_RGB332);

   ...

   tbm_surface_destroy (surface);
   @endcode
 */
int tbm_surface_destroy (tbm_surface_h surface);

/**
 * @brief Maps the tbm_surface according to the access option.
 * @details After mapping tbm_surface, the information of tbm_surface is assigned in #tbm_surface_info_s struct.\n
 * The information of tbm_surface has width, height, format, bpp, size, number of planes and information of planes.\n
 * The information of planes has stride, offset, size and pointer of plane\n
 * #TBM_SURF_OPTION_READ indecates access option to read.\n
 * #TBM_SURF_OPTION_WRITE indecates access option to write.
 * @since_tizen 2.3
 * @param[in] surface : the tbm_surface_h
 * @param[in] opt : the option to access the tbm_surface
 * @param[out] *info : the infomation of the tbm_surface
 * @return #TBM_SURFACE_ERROR_NONE if this function succeeds, otherwise error status value
 * @retval #TBM_SURFACE_ERROR_NONE
 * @retval #TBM_SURFACE_ERROR_INVALID_PARAMETER
 * @retval #TBM_SURFACE_ERROR_INVALID_OPERATION
 * @see tbm_surface_unmap();
 * @par Example
   @code
   #include <tbm_surface.h>

   tbm_surface_h surface;
   tbm_surface_info_s info;
   int ret;

   surface = tbm_surface_create (128, 128, TBM_FORMAT_RGB332);
   ret = tbm_surface_map (surface, TBM_SURF_OPTION_WRITE|TBM_SURF_OPTION_READ, &info);

   ...

   tbm_surface_unmap (surface);
   tbm_surface_destroy (surface);
   @endcode
 */
int tbm_surface_map (tbm_surface_h surface, int opt, tbm_surface_info_s *info);

/**
 * @brief Unmaps the tbm_surface.
 * @since_tizen 2.3
 * @param[in] surface : the tbm_surface_h
 * @return #TBM_SURFACE_ERROR_NONE if this function succeeds, otherwise error status value
 * @retval #TBM_SURFACE_ERROR_NONE
 * @retval #TBM_SURFACE_ERROR_INVALID_PARAMETER
 * @see tbm_surface_map()
 * @par Example
   @code
   #include <tbm_surface.h>

   tbm_surface_h surface;
   tbm_surface_info_s info;
   int ret;

   surface = tbm_surface_create (128, 128, TBM_FORMAT_RGB332);
   ret = tbm_surface_map (surface, TBM_SURF_OPTION_WRITE|TBM_SURF_OPTION_READ, &info);

   ...

   tbm_surface_unmap (surface);
   tbm_surface_destroy (surface);
   @endcode
 */
int tbm_surface_unmap (tbm_surface_h surface);

/**
 * @brief Gets the information of the tbm_surface.
 * @details The information of tbm_surface is assigned in #tbm_surface_info_s struct.\n
 * The information of tbm_surface has width, height, format, bpp, size, number of planes and information of planes.\n
 * The information of planes has stride, offset, size and pointer of plane.
 * @since_tizen 2.3
 * @param[in] surface : the tbm_surface_h
 * @param[out] *info : the infomation of the tbm_surface
 * @return #TBM_SURFACE_ERROR_NONE if this function succeeds, otherwise error status value
 * @retval #TBM_SURFACE_ERROR_NONE
 * @retval #TBM_SURFACE_ERROR_INVALID_PARAMETER
 * @retval #TBM_SURFACE_ERROR_INVALID_OPERATION
 * @see tbm_surface_map()
 * @par Example
   @code
   #include <tbm_surface.h>

   tbm_surface_h surface;
   tbm_surface_info_s info;
   int ret;

   surface = tbm_surface_create (128, 128, TBM_FORMAT_RGB332);
   ret = tbm_surface_get_info (surface, &info);

   ...

   tbm_surface_destroy (surface);
   @endcode
 */
int tbm_surface_get_info (tbm_surface_h surface, tbm_surface_info_s *info);

/**
 * @brief Gets the width of the tbm_surface.
 * @since_tizen 2.3
 * @param[in] surface : the tbm_surface_h
 * @return the width of the tbm_surface if this function succeeds, otherwise error status value
 * @retval #TBM_SURFACE_ERROR_INVALID_PARAMETER
 * @par Example
   @code
   #include <tbm_surface.h>

   tbm_surface_h surface;
   int width;

   surface = tbm_surface_create (128, 128, TBM_FORMAT_RGB332);

   ...

   width = tbm_surface_get_width (surface);

   ...

   tbm_surface_destroy (surface);
   @endcode
 */
int tbm_surface_get_width (tbm_surface_h surface);

/**
 * @brief Gets the height of the tbm_surface.
 * @since_tizen 2.3
 * @param[in] surface : the tbm_surface_h
 * @return the height of the tbm_surface if this function succeeds, otherwise error status value
 * @retval #TBM_SURFACE_ERROR_INVALID_PARAMETER
 * @par Example
   @code
   #include <tbm_surface.h>

   tbm_surface_h surface;
   int height;

   surface = tbm_surface_create (128, 128, TBM_FORMAT_RGB332);

   ...

   height = tbm_surface_get_height (surface);

   ...

   tbm_surface_destroy (surface);
   @endcode
 */
int tbm_surface_get_height (tbm_surface_h surface);

/**
 * @brief Gets the format of the tbm_surface.
 * @since_tizen 2.3
 * @remark The specific error code can be obtained using the get_last_result() method. Error codes are described in Exception section.
 * @param[in] surface : the tbm_surface_h
 * @return the format of the tbm_surface if this function succeeds, otherwise 0
 * @retval #tbm_format
 * @exception #TBM_SURFACE_ERROR_NONE Success
 * @exception #TBM_SURFACE_ERROR_INVALID_PARAMETER  Invalid parameter
 * @par Example
   @code
   #include <tbm_surface.h>

   tbm_surface_s surface;
   tbm_format format;

   surface = tbm_surface_create (128, 128, TBM_FORMAT_RGB332);

   ...

   format = tbm_surface_get_format (surface);

   ...

   tbm_surface_destroy (surface);
   @endcode
 */
tbm_format tbm_surface_get_format (tbm_surface_h surface);

#ifdef __cplusplus
}
#endif

/**
* @}
*/

#endif /* _TBM_SURFACE_H_ */

