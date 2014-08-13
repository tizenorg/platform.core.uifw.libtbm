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

#include "config.h"

#include "tbm_bufmgr.h"
#include "tbm_bufmgr_int.h"
#include <xf86drm.h>
#include <X11/Xmd.h>
#include <dri2.h>

int
tbm_bufmgr_get_drm_fd_x11()
{
    int screen;
    Display *display;
    int dri2Major, dri2Minor;
    int eventBase, errorBase;
    drm_magic_t magic;
    char *driver_name, *device_name;
    int fd;

    display = XOpenDisplay(NULL);
    if (!display)
    {
        TBM_LOG ("[libtbm:%d] Fail XOpenDisplay\n", getpid());
        return -1;
    }

    screen = DefaultScreen(display);

    if (!DRI2QueryExtension (display, &eventBase, &errorBase))
    {
        TBM_LOG ("[libtbm:%d] Fail DRI2QueryExtention\n", getpid());
        XCloseDisplay(display);
        return -1;
    }

    if (!DRI2QueryVersion (display, &dri2Major, &dri2Minor))
    {
        TBM_LOG ("[libtbm:%d] Fail DRI2QueryVersion\n", getpid());
        XCloseDisplay(display);
        return -1;
    }

    if (!DRI2Connect (display, RootWindow(display, screen), &driver_name, &device_name))
    {
        TBM_LOG ("[libtbm:%d] Fail DRI2Connect\n", getpid());
        XCloseDisplay(display);
        return -1;
    }

    fd = open (device_name, O_RDWR);
    if (fd < 0)
    {
        TBM_LOG ("[libtbm:%d] cannot open drm device (%s)\n", getpid(), device_name);
        free (driver_name);
        free (device_name);
        XCloseDisplay(display);
        return -1;
    }

    if (drmGetMagic (fd, &magic))
    {
        TBM_LOG ("[libtbm:%d] Fail drmGetMagic\n", getpid());
        free (driver_name);
        free (device_name);
        close(fd);
        XCloseDisplay(display);
        return -1;
    }

    if (!DRI2Authenticate(display, RootWindow(display, screen), magic))
    {
        TBM_LOG ("[libtbm:%d] Fail DRI2Authenticate\n", getpid());
        free (driver_name);
        free (device_name);
        close(fd);
        XCloseDisplay(display);
        return -1;
    }

    if(!drmAuthMagic(fd, magic))
    {
        TBM_LOG ("[libtbm:%d] Fail drmAuthMagic\n", getpid());
        free (driver_name);
        free (device_name);
        close(fd);
        XCloseDisplay(display);
        return -1;
    }

    free (driver_name);
    free (device_name);
    XCloseDisplay(display);

    return fd;
}

