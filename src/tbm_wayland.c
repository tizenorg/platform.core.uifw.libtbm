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

#include <stdint.h>
#include <stddef.h>
#include "wayland-client.h"

struct wl_client;
struct wl_resource;

struct wl_drm;

#ifndef WL_DRM_ERROR_ENUM
#define WL_DRM_ERROR_ENUM
enum wl_drm_error {
	WL_DRM_ERROR_AUTHENTICATE_FAIL = 0,
	WL_DRM_ERROR_INVALID_FORMAT = 1,
	WL_DRM_ERROR_INVALID_NAME = 2,
};
#endif /* WL_DRM_ERROR_ENUM */


#ifndef WL_DRM_CAPABILITY_ENUM
#define WL_DRM_CAPABILITY_ENUM
/**
 * wl_drm_capability - wl_drm capability bitmask
 * @WL_DRM_CAPABILITY_PRIME: wl_drm prime available
 *
 * Bitmask of capabilities.
 */
enum wl_drm_capability {
	WL_DRM_CAPABILITY_PRIME = 1,
};
#endif /* WL_DRM_CAPABILITY_ENUM */

struct wl_drm_listener {
	/**
	 * device - (none)
	 * @name: (none)
	 */
	void (*device)(void *data,
		       struct wl_drm *wl_drm,
		       const char *name);
	/**
	 * format - (none)
	 * @format: (none)
	 */
	void (*format)(void *data,
		       struct wl_drm *wl_drm,
		       uint32_t format);
	/**
	 * authenticated - (none)
	 */
	void (*authenticated)(void *data,
			      struct wl_drm *wl_drm);
	/**
	 * capabilities - (none)
	 * @value: (none)
	 */
	void (*capabilities)(void *data,
			     struct wl_drm *wl_drm,
			     uint32_t value);
};

static inline int
wl_drm_add_listener(struct wl_drm *wl_drm,
		    const struct wl_drm_listener *listener, void *data)
{
	return wl_proxy_add_listener((struct wl_proxy *) wl_drm,
				     (void (**)(void)) listener, data);
}

#define WL_DRM_AUTHENTICATE	0

static inline void
wl_drm_destroy(struct wl_drm *wl_drm)
{
	wl_proxy_destroy((struct wl_proxy *) wl_drm);
}

static inline void
wl_drm_authenticate(struct wl_drm *wl_drm, uint32_t id)
{
	wl_proxy_marshal((struct wl_proxy *) wl_drm,
			 WL_DRM_AUTHENTICATE, id);
}

static const struct wl_interface *types[] = {
	NULL,
	&wl_buffer_interface,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	&wl_buffer_interface,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	&wl_buffer_interface,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
};

static const struct wl_message wl_drm_requests[] = {
	{ "authenticate", "u", types + 0 },
	{ "create_buffer", "nuiiuu", types + 1 },
	{ "create_planar_buffer", "nuiiuiiiiii", types + 7 },
	{ "create_prime_buffer", "2nhiiuiiiiii", types + 18 },
};

static const struct wl_message wl_drm_events[] = {
	{ "device", "s", types + 0 },
	{ "format", "u", types + 0 },
	{ "authenticated", "", types + 0 },
	{ "capabilities", "u", types + 0 },
};

static const struct wl_interface wl_drm_interface = {
	"wl_drm", 2,
	4, wl_drm_requests,
	4, wl_drm_events,
};

#define USE_QUEUE 1

struct wl_drm_info {
#if USE_QUEUE
   struct wl_event_queue *wl_queue;
#endif   
   struct wl_drm* wl_drm;
   int authenticated;
   int fd;
};

static void wl_client_drm_handle_device(void *data, struct wl_drm *drm, const char *device)
{
    struct wl_drm_info *drm_info = (struct wl_drm_info *)data;
    drm_magic_t magic;

    printf("device[%s]\n", device);
    drm_info->fd = open(device, O_RDWR | O_CLOEXEC);
    if (drm_info->fd < 0) {
        printf("Failed to open a device: %d (%s)\n", errno, device);
        return;
    }

    drmGetMagic(drm_info->fd, &magic);
    printf("magic[%x]\n", magic);
    wl_drm_authenticate(drm_info->wl_drm, magic);
}

static void wl_client_drm_handle_format(void *data, struct wl_drm *drm, uint32_t format)
{
    /* Do nothing */
}

static void wl_client_drm_handle_authenticated(void *data, struct wl_drm *drm)
{
    struct wl_drm_info *drm_info = (struct wl_drm_info *)data;
    drm_info->authenticated = 1;
}

static void wl_client_drm_handle_capabilities(void *data, struct wl_drm *drm, uint32_t value)
{
    /* Do nothing */
}

 

static void wl_client_registry_handle_global(void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version)
{
    struct wl_drm_info *info = (struct wl_drm_info *)data;
    static const struct wl_drm_listener wl_drm_client_listener = {
        wl_client_drm_handle_device,
        wl_client_drm_handle_format,
        wl_client_drm_handle_authenticated,
        wl_client_drm_handle_capabilities
    };

    printf("interface[%s]\n", interface);
    if (!strcmp(interface, "wl_drm")) {
        info->wl_drm = wl_registry_bind(registry, name, &wl_drm_interface, (version > 2) ? 2 : version);
#if USE_QUEUE        
        wl_proxy_set_queue((struct wl_proxy *)info->wl_drm, info->wl_queue);
#endif        
        wl_drm_add_listener(info->wl_drm, &wl_drm_client_listener, data);
    }
}

static int tbm_util_get_drm_fd(void *dpy, int *fd)
{
    struct wl_display *disp = NULL;
    struct wl_registry *wl_registry;
    int ret = 0;
    struct wl_drm_info info = {
#if USE_QUEUE    
        .wl_queue = NULL,
#endif
        .wl_drm = NULL,
        .authenticated = 0,
        .fd = -1,
    };
    static const struct wl_registry_listener registry_listener = {
        wl_client_registry_handle_global,
        NULL
    };

    if (!fd) {
        return -1;
    }

    if (!dpy) {
        disp = wl_display_connect(NULL);
        if (!disp) {
            printf("Failed to create a new display connection\n");
            return -1;
        }
        dpy = disp;
    }
    
#if USE_QUEUE
    info.wl_queue = wl_display_create_queue(dpy);
    if (!info.wl_queue) {
        printf("Failed to create a WL Queue\n");
        if (disp == dpy) {
            wl_display_disconnect(disp);
        }
        return -1;
    }
#endif
    wl_registry = wl_display_get_registry(dpy);
    if (!wl_registry) {
        printf("Failed to get registry\n");
#if USE_QUEUE        
        wl_event_queue_destroy(info.wl_queue);
#endif
        if (disp == dpy) {
            wl_display_disconnect(disp);
        }
        return -1;
    }
#if USE_QUEUE
    wl_proxy_set_queue((struct wl_proxy *)wl_registry, info.wl_queue);
#endif
    wl_registry_add_listener(wl_registry, &registry_listener, &info); 
    wl_display_roundtrip(dpy);
    
    printf("Consuming Dispatch Queue begin\n");
    while (ret != -1 && !info.authenticated) {
#if USE_QUEUE
        ret = wl_display_dispatch_queue(dpy, info.wl_queue);
#else
        ret = wl_display_dispatch(dpy);
#endif
        printf("Dispatch Queue consumed: %d\n", ret);
    }
    printf("Consuming Dispatch Queue end\n");
    
#if USE_QUEUE
    wl_event_queue_destroy(info.wl_queue);
#endif    
    wl_registry_destroy(wl_registry);
    wl_drm_destroy(info.wl_drm);

    *fd = info.fd;
    if (disp == dpy) {
        wl_display_disconnect(disp);
    }
    return *fd >= 0 ? 0 : -1;
}

int
tbm_bufmgr_get_drm_fd_wayland()
{
    int fd = -1;
    
    if(tbm_util_get_drm_fd(NULL, &fd))
    {
        printf("Failed to get drm_fd\n");
    }
    
    return fd;
}