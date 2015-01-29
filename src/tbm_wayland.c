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

#include "tbm_bufmgr_int.h"
#include <xf86drm.h>
#include <wayland-client.h>
#include <wayland-drm-client-protocol.h>

struct display {
	struct wl_display *display;
	struct wl_registry *registry;

	struct wl_drm	*wl_drm;
	uint32_t		drm_fd;
	char*			drm_device_name;
};

/* initialize drm device */
static void
_drm_handle_device( void *data, struct wl_drm *drm, const char *device )
{
	struct display *d = data;

	d->drm_device_name = strdup(device);
	return;
}

/* handle format of drm device */
static void
_drm_handle_format( void *data, struct wl_drm *drm, uint32_t format )
{
	return;
}

/* handle authentication of drm device */
static void
_drm_handle_authenticated( void *data, struct wl_drm *drm )
{
	struct display *d = data;

	printf("DRM authenticated: name:%s fd:%d\n", d->drm_device_name, d->drm_fd);
	return;
}

static void
_drm_handle_capabilities(void *data, struct wl_drm *drm, uint32_t value)
{
	return;
}

static const struct wl_drm_listener drm_listener = {
	_drm_handle_device,
	_drm_handle_format,
	_drm_handle_authenticated,
	_drm_handle_capabilities
};


static void
registry_handle_global(void *data, struct wl_registry *registry,
		       uint32_t id, const char *interface, uint32_t version)
{
	struct display *d = data;

	if (strcmp(interface, "wl_drm") == 0) {
		d->wl_drm =
			wl_registry_bind(registry, id, &wl_drm_interface, version);
		wl_drm_add_listener(d->wl_drm, &drm_listener, d);
	}
}

static void
registry_handle_global_remove(void *data, struct wl_registry *registry,
			      uint32_t name)
{
}

static const struct wl_registry_listener registry_listener = {
	registry_handle_global,
	registry_handle_global_remove
};

static struct display *
create_display(void)
{
	struct display *display;
    drm_magic_t magic;

	display = malloc(sizeof *display);
	if (display == NULL) {
        TBM_LOG ("[libtbm:%d] out of memory\n", getpid());
        return NULL;
	}
	display->display = wl_display_connect(NULL);
    if (display->display == NULL) {
        TBM_LOG ("[libtbm:%d] fail to wl display\n", getpid());
        return NULL;
    }

	display->registry = wl_display_get_registry(display->display);
	wl_registry_add_listener(display->registry,
				 &registry_listener, display);
	wl_display_roundtrip(display->display);

    if (display->wl_drm == NULL) {
        TBM_LOG ("[libtbm:%d] No wl_drm global\n", getpid());
        return NULL;
    }
    wl_display_roundtrip(display->display);

    if (!display->drm_device_name) {
        TBM_LOG ("[libtbm:%d] No drm device name\n", getpid());
        return NULL;
    }

    display->drm_fd = open( display->drm_device_name, O_RDWR | O_CLOEXEC );
    if (display->drm_fd < 0) {
        TBM_LOG ("[libtbm:%d] Cannot open drm device\n", getpid());
        return NULL;
    }

    drmGetMagic(display->drm_fd, &magic);
    wl_drm_authenticate(display->wl_drm, magic);
    wl_display_roundtrip(display->display);

	return display;
}

static void
destroy_display(struct display *display)
{
	if (display->wl_drm)
		wl_drm_destroy(display->wl_drm);
	wl_registry_destroy(display->registry);
	wl_display_flush(display->display);
	wl_display_disconnect(display->display);
	if (display->drm_device_name)
		free (display->drm_device_name);
	free(display);
}

int
tbm_bufmgr_get_drm_fd_wayland()
{
	struct display *display = NULL;
	int drm_fd;

	display = create_display();
    if (display == NULL) {
        TBM_LOG ("[libtbm:%d] fail to create display\n", getpid());
        return -1;
    }

	drm_fd = display->drm_fd;
	destroy_display(display);

    return drm_fd;
}

