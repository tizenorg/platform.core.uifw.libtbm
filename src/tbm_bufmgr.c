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
#include "tbm_bufmgr_backend.h"
#include "list.h"

#ifdef DEBUG
int bDebug;
#endif

#ifdef HAVE_DLOG
int bDlog;
#endif

#define PREFIX_LIB    "libtbm_"
#define SUFFIX_LIB    ".so"
#define DEFAULT_LIB   PREFIX_LIB"default"SUFFIX_LIB

/* values to indicate unspecified fields in XF86ModReqInfo. */
#define MAJOR_UNSPEC        0xFF
#define MINOR_UNSPEC        0xFF
#define PATCH_UNSPEC        0xFFFF
#define ABI_VERS_UNSPEC   0xFFFFFFFF

#define MODULE_VERSION_NUMERIC(maj, min, patch) \
			((((maj) & 0xFF) << 24) | (((min) & 0xFF) << 16) | (patch & 0xFFFF))
#define GET_MODULE_MAJOR_VERSION(vers)    (((vers) >> 24) & 0xFF)
#define GET_MODULE_MINOR_VERSION(vers)    (((vers) >> 16) & 0xFF)
#define GET_MODULE_PATCHLEVEL(vers)    ((vers) & 0xFFFF)

enum {
	LOCK_TRY_ONCE,
	LOCK_TRY_ALWAYS,
	LOCK_TRY_NEVER
};

pthread_mutex_t gLock = PTHREAD_MUTEX_INITIALIZER;
tbm_bufmgr gBufMgr;

static __thread tbm_error_e tbm_last_error = TBM_ERROR_NONE;

static void
_tbm_set_last_result(tbm_error_e err)
{
	tbm_last_error = err;
}

/* LCOV_EXCL_START */
static void
_tbm_util_get_appname_brief(char *brief)
{
	char delim[] = "/";
	char *token = NULL;
	char temp[255] = {0,};
	char *saveptr = NULL;

	token = strtok_r(brief, delim, &saveptr);

	while (token != NULL) {
		memset(temp, 0x00, 255 * sizeof(char));
		strncpy(temp, token, 254 * sizeof(char));
		token = strtok_r(NULL, delim, &saveptr);
	}

	snprintf(brief, sizeof(temp), "%s", temp);
}

static void
_tbm_util_get_appname_from_pid(long pid, char *str)
{
	FILE *fp;
	int len;
	long app_pid = pid;
	char fn_cmdline[255] = {0,};
	char cmdline[255] = {0,};

	snprintf(fn_cmdline, sizeof(fn_cmdline), "/proc/%ld/cmdline", app_pid);

	fp = fopen(fn_cmdline, "r");
	if (fp == 0) {
		fprintf(stderr, "cannot file open /proc/%ld/cmdline", app_pid);
		return;
	}

	if (!fgets(cmdline, 255, fp)) {
		fprintf(stderr, "fail to get appname for pid(%ld)\n", app_pid);
		fclose(fp);
		return;
	}
	fclose(fp);

	len = strlen(cmdline);
	if (len < 1)
		memset(cmdline, 0x00, 255);
	else
		cmdline[len] = 0;

	snprintf(str, sizeof(cmdline), "%s", cmdline);
}
/* LCOV_EXCL_STOP */

tbm_user_data
*user_data_lookup(struct list_head *user_data_list, unsigned long key)
{
	tbm_user_data *user_data = NULL;
	tbm_user_data *old_data = NULL, *tmp = NULL;

	if (!LIST_IS_EMPTY(user_data_list)) {
		LIST_FOR_EACH_ENTRY_SAFE(old_data, tmp, user_data_list, item_link) {
			if (old_data->key == key) {
				user_data = old_data;
				return user_data;
			}
		}
	}

	return user_data;
}

tbm_user_data
*user_data_create(unsigned long key, tbm_data_free data_free_func)
{
	tbm_user_data *user_data = NULL;

	user_data = calloc(1, sizeof(tbm_user_data));
	if (!user_data)
		return NULL;

	user_data->key = key;
	user_data->free_func = data_free_func;
	user_data->data = (void *)0;

	return user_data;
}

void
user_data_delete(tbm_user_data *user_data)
{
	if (user_data->data && user_data->free_func)
		user_data->free_func(user_data->data);

	LIST_DEL(&user_data->item_link);

	free(user_data);
}

static int
_bo_lock(tbm_bo bo, int device, int opt)
{
	tbm_bufmgr bufmgr = bo->bufmgr;
	int ret = 0;

	if (bufmgr->backend->bo_lock) {
		ret = bufmgr->backend->bo_lock(bo, device, opt);
	} else {
		ret = 1;
	}

	return ret;
}

static void
_bo_unlock(tbm_bo bo)
{
	tbm_bufmgr bufmgr = bo->bufmgr;

	if (bufmgr->backend->bo_unlock) {
		bufmgr->backend->bo_unlock(bo);
	}
}

static int
_tbm_bo_lock(tbm_bo bo, int device, int opt)
{
	tbm_bufmgr bufmgr = NULL;
	int old;
	int ret = 0;

	if (!bo)
		return 0;

	bufmgr = bo->bufmgr;

	/* do not try to lock the bo */
	if (bufmgr->lock_type == LOCK_TRY_NEVER)
		return 1;

	if (bo->lock_cnt < 0) {
		TBM_LOG_E("error bo:%p LOCK_CNT=%d\n",
			bo, bo->lock_cnt);
	}

	old = bo->lock_cnt;
	if (bufmgr->lock_type == LOCK_TRY_ONCE) {
		if (bo->lock_cnt == 0) {
			pthread_mutex_unlock(&bufmgr->lock);
			ret = _bo_lock(bo, device, opt);
			pthread_mutex_lock(&bufmgr->lock);
			if (ret)
				bo->lock_cnt++;
		} else
			ret = 1;
	} else if (bufmgr->lock_type == LOCK_TRY_ALWAYS) {
		pthread_mutex_unlock(&bufmgr->lock);
		ret = _bo_lock(bo, device, opt);
		pthread_mutex_lock(&bufmgr->lock);
		if (ret)
			bo->lock_cnt++;
	} else {
		TBM_LOG_E("error bo:%p lock_type is wrong.\n",
			bo);
	}

	DBG_LOCK(">> LOCK bo:%p(%d->%d)\n",
		 bo, old, bo->lock_cnt);

	return ret;
}

static void
_tbm_bo_unlock(tbm_bo bo)
{
	tbm_bufmgr bufmgr = NULL;

	int old;

	if (!bo)
		return;

	bufmgr = bo->bufmgr;

	/* do not try to unlock the bo */
	if (bufmgr->lock_type == LOCK_TRY_NEVER)
		return;

	old = bo->lock_cnt;
	if (bufmgr->lock_type == LOCK_TRY_ONCE) {
		if (bo->lock_cnt > 0) {
			bo->lock_cnt--;
			if (bo->lock_cnt == 0)
				_bo_unlock(bo);
		}
	} else if (bufmgr->lock_type == LOCK_TRY_ALWAYS) {
		if (bo->lock_cnt > 0) {
			bo->lock_cnt--;
			_bo_unlock(bo);
		}
	} else {
		TBM_LOG_E("error bo:%p lock_type is wrong.\n",
			bo);
	}

	if (bo->lock_cnt < 0)
		bo->lock_cnt = 0;

	DBG_LOCK(">> UNLOCK bo:%p(%d->%d)\n",
		 bo, old, bo->lock_cnt);
}

static int
_tbm_bo_is_valid(tbm_bo bo)
{
	tbm_bo old_data = NULL, tmp = NULL;

	if (bo == NULL)
		return 0;

	if (!LIST_IS_EMPTY(&gBufMgr->bo_list)) {
		LIST_FOR_EACH_ENTRY_SAFE(old_data, tmp, &gBufMgr->bo_list, item_link) {
			if (old_data == bo)
				return 1;
		}

	}
	return 0;
}

static void
_tbm_bo_ref(tbm_bo bo)
{
	bo->ref_cnt++;
}

static void
_tbm_bo_unref(tbm_bo bo)
{
	tbm_bufmgr bufmgr = bo->bufmgr;
	tbm_user_data *old_data = NULL, *tmp = NULL;

	if (bo->ref_cnt <= 0)
		return;

	bo->ref_cnt--;
	if (bo->ref_cnt == 0) {
		/* destory the user_data_list */
		if (!LIST_IS_EMPTY(&bo->user_data_list)) {
			LIST_FOR_EACH_ENTRY_SAFE(old_data, tmp, &bo->user_data_list, item_link) {
				DBG("free user_data\n");
				user_data_delete(old_data);
			}
		}

		if (bo->lock_cnt > 0) {
			TBM_LOG_E("error lock_cnt:%d\n",
				bo->lock_cnt);
			_bo_unlock(bo);
		}

		/* call the bo_free */
		bufmgr->backend->bo_free(bo);
		bo->priv = NULL;

		LIST_DEL(&bo->item_link);
		free(bo);
		bo = NULL;
	}

}

/* LCOV_EXCL_START */
static int
_check_version(TBMModuleVersionInfo *data)
{
	int abimaj, abimin;
	int vermaj, vermin;

	abimaj = GET_ABI_MAJOR(data->abiversion);
	abimin = GET_ABI_MINOR(data->abiversion);

	DBG("TBM module %s: vendor=\"%s\" ABI=%d,%d\n",
	    data->modname ? data->modname : "UNKNOWN!",
	    data->vendor ? data->vendor : "UNKNOWN!", abimaj, abimin);

	vermaj = GET_ABI_MAJOR(TBM_ABI_VERSION);
	vermin = GET_ABI_MINOR(TBM_ABI_VERSION);

	DBG("TBM ABI version %d.%d\n",
	    vermaj, vermin);

	if (abimaj != vermaj) {
		TBM_LOG_E("TBM module ABI major ver(%d) doesn't match the TBM's ver(%d)\n",
			abimaj, vermaj);
		return 0;
	} else if (abimin > vermin) {
		TBM_LOG_E("TBM module ABI minor ver(%d) is newer than the TBM's ver(%d)\n",
			abimin, vermin);
		return 0;
	}
	return 1;
}

static int
_tbm_bufmgr_load_module(tbm_bufmgr bufmgr, int fd, const char *file)
{
	char path[PATH_MAX] = { 0, };
	TBMModuleData *initdata = NULL;
	void *module_data;

	snprintf(path, sizeof(path), BUFMGR_MODULE_DIR "/%s", file);

	module_data = dlopen(path, RTLD_LAZY);
	if (!module_data) {
		TBM_LOG_E("failed to load module: %s(%s)\n",
			dlerror(), file);
		return 0;
	}

	initdata = dlsym(module_data, "tbmModuleData");
	if (initdata) {
		ModuleInitProc init;
		TBMModuleVersionInfo *vers;

		vers = initdata->vers;
		init = initdata->init;

		if (vers) {
			if (!_check_version(vers)) {
				dlclose(module_data);
				return 0;
			}
		} else {
			TBM_LOG_E("Error: module does not supply version information.\n");

			dlclose(module_data);
			return 0;
		}

		if (init) {
			if (!init(bufmgr, fd)) {
				TBM_LOG_E("Fail to init module(%s)\n",
					file);
				dlclose(module_data);
				return 0;
			}

			if (!bufmgr->backend || !bufmgr->backend->priv) {
				TBM_LOG_E("Error: module(%s) wrong operation. Check backend or backend's priv.\n",
					file);
				dlclose(module_data);
				return 0;
			}
		} else {
			TBM_LOG_E("Error: module does not supply init symbol.\n");
			dlclose(module_data);
			return 0;
		}
	} else {
		TBM_LOG_E("Error: module does not have data object.\n");
		dlclose(module_data);
		return 0;
	}

	bufmgr->module_data = module_data;

	DBG("Success to load module(%s)\n",
	    file);

	return 1;
}

static int
_tbm_load_module(tbm_bufmgr bufmgr, int fd)
{
	struct dirent **namelist;
	const char *p = NULL;
	int n;
	int ret = 0;

	/* load bufmgr priv from default lib */
	ret = _tbm_bufmgr_load_module(bufmgr, fd, DEFAULT_LIB);

	/* load bufmgr priv from configured path */
	if (!ret) {
		n = scandir(BUFMGR_MODULE_DIR, &namelist, 0, alphasort);
		if (n < 0) {
			TBM_LOG_E("no files : %s\n",
				BUFMGR_MODULE_DIR);
		} else {
			while (n--) {
				if (!ret && strstr(namelist[n]->d_name, PREFIX_LIB)) {
					p = strstr(namelist[n]->d_name, SUFFIX_LIB);
					if (p && !strcmp(p, SUFFIX_LIB))
						ret = _tbm_bufmgr_load_module(bufmgr, fd, namelist[n]->d_name);
				}
				free(namelist[n]);
			}
			free(namelist);
		}
	}

	return ret;
}
/* LCOV_EXCL_STOP */

tbm_bufmgr
tbm_bufmgr_init(int fd)
{
	char *env;

	pthread_mutex_lock(&gLock);

	/* LCOV_EXCL_START */
#ifdef HAVE_DLOG
	env = getenv("TBM_DLOG");
	if (env) {
		bDlog = atoi(env);
		TBM_LOG_D("TBM_DLOG=%s\n", env);
	} else {
		bDlog = 0;
	}
#endif

#ifdef DEBUG
	env = getenv("TBM_DEBUG");
	if (env) {
		bDebug = atoi(env);
		TBM_LOG_D("TBM_DEBUG=%s\n", env);
	} else {
		bDebug = 0;
	}
#endif
	/* LCOV_EXCL_STOP */

	/* initialize buffer manager */
	if (gBufMgr) {
		gBufMgr->ref_count++;

		DBG("bufmgr:%p ref: fd=%d, ref_count:%d\n",
		    gBufMgr, gBufMgr->fd, gBufMgr->ref_count);
		pthread_mutex_unlock(&gLock);
		return gBufMgr;
	}

	DBG("bufmgr init\n");

	/* allocate bufmgr */
	gBufMgr = calloc(1, sizeof(struct _tbm_bufmgr));
	if (!gBufMgr) {
		_tbm_set_last_result(TBM_BO_ERROR_HEAP_ALLOC_FAILED);
		pthread_mutex_unlock(&gLock);
		return NULL;
	}

	gBufMgr->fd = fd;

	/* load bufmgr priv from env */
	if (!_tbm_load_module(gBufMgr, gBufMgr->fd)) {
		/* LCOV_EXCL_START */
		_tbm_set_last_result(TBM_BO_ERROR_LOAD_MODULE_FAILED);
		TBM_LOG_E("error : Fail to load bufmgr backend\n");

		free(gBufMgr);
		gBufMgr = NULL;
		pthread_mutex_unlock(&gLock);
		return NULL;
		/* LCOV_EXCL_STOP */
	}

	/* log for tbm backend_flag */
	DBG("backend flag:%x:", gBufMgr->backend->flags);
	DBG("\n");

	gBufMgr->ref_count = 1;

	DBG("create tizen bufmgr:%p ref_count:%d\n",
	    gBufMgr, gBufMgr->ref_count);

	if (pthread_mutex_init(&gBufMgr->lock, NULL) != 0) {
		/* LCOV_EXCL_START */
		_tbm_set_last_result(TBM_BO_ERROR_THREAD_INIT_FAILED);
		gBufMgr->backend->bufmgr_deinit(gBufMgr->backend->priv);
		tbm_backend_free(gBufMgr->backend);
		dlclose(gBufMgr->module_data);
		free(gBufMgr);
		gBufMgr = NULL;
		pthread_mutex_unlock(&gLock);
		return NULL;
		/* LCOV_EXCL_STOP */
	}

	/* setup the lock_type */
	env = getenv("BUFMGR_LOCK_TYPE");
	if (env && !strcmp(env, "always"))
		gBufMgr->lock_type = LOCK_TRY_ALWAYS;
	else if (env && !strcmp(env, "none"))
		gBufMgr->lock_type = LOCK_TRY_NEVER;
	else if (env && !strcmp(env, "once"))
		gBufMgr->lock_type = LOCK_TRY_ONCE;
	else
		gBufMgr->lock_type = LOCK_TRY_ALWAYS;

	DBG("BUFMGR_LOCK_TYPE=%s\n",
	    env ? env : "default:once");

	/* intialize bo_list */
	LIST_INITHEAD(&gBufMgr->bo_list);

	/* intialize surf_list */
	LIST_INITHEAD(&gBufMgr->surf_list);

	pthread_mutex_unlock(&gLock);
	return gBufMgr;
}

void
tbm_bufmgr_deinit(tbm_bufmgr bufmgr)
{
	TBM_RETURN_IF_FAIL(TBM_BUFMGR_IS_VALID(bufmgr));

	tbm_bo bo = NULL;
	tbm_bo tmp = NULL;

	tbm_surface_h surf = NULL;
	tbm_surface_h tmp_surf = NULL;

	pthread_mutex_lock(&gLock);

	if (!gBufMgr) {
		TBM_LOG_E("gBufmgr already destroy: bufmgr:%p\n", bufmgr);
		pthread_mutex_unlock(&gLock);
		return;
	}

	bufmgr->ref_count--;
	if (bufmgr->ref_count > 0) {
		DBG("tizen bufmgr destroy: bufmgr:%p, ref_count:%d\n",
			bufmgr, bufmgr->ref_count);
		pthread_mutex_unlock(&gLock);
		return;
	}

	/* destroy bo_list */
	if (!LIST_IS_EMPTY(&bufmgr->bo_list)) {
		LIST_FOR_EACH_ENTRY_SAFE(bo, tmp, &bufmgr->bo_list, item_link) {
			TBM_LOG_E("Un-freed bo(%p, ref:%d)\n",
				bo, bo->ref_cnt);
			bo->ref_cnt = 1;
			tbm_bo_unref(bo);
		}
	}

	/* destroy surf_list */
	if (!LIST_IS_EMPTY(&bufmgr->surf_list)) {
		LIST_FOR_EACH_ENTRY_SAFE(surf, tmp_surf, &bufmgr->surf_list, item_link) {
			TBM_LOG_E("Un-freed surf(%p, ref:%d)\n",
				surf, surf->refcnt);
			tbm_surface_destroy(surf);
		}
	}

	/* destroy bufmgr priv */
	bufmgr->backend->bufmgr_deinit(bufmgr->backend->priv);
	bufmgr->backend->priv = NULL;
	tbm_backend_free(bufmgr->backend);
	bufmgr->backend = NULL;

	pthread_mutex_destroy(&bufmgr->lock);

	DBG("tizen bufmgr destroy: bufmgr:%p\n",
	    bufmgr);

	dlclose(bufmgr->module_data);

	if (bufmgr->fd > 0)
		close(bufmgr->fd);

	free(bufmgr);
	bufmgr = NULL;
	gBufMgr = NULL;

	pthread_mutex_unlock(&gLock);
}

int
tbm_bo_size(tbm_bo bo)
{
	TBM_RETURN_VAL_IF_FAIL(_tbm_bo_is_valid(bo), 0);

	tbm_bufmgr bufmgr = bo->bufmgr;
	int size;

	pthread_mutex_lock(&bufmgr->lock);

	size = bufmgr->backend->bo_size(bo);

	pthread_mutex_unlock(&bufmgr->lock);

	return size;
}

tbm_bo
tbm_bo_ref(tbm_bo bo)
{
	TBM_RETURN_VAL_IF_FAIL(_tbm_bo_is_valid(bo), NULL);

	tbm_bufmgr bufmgr = bo->bufmgr;

	pthread_mutex_lock(&bufmgr->lock);

	_tbm_bo_ref(bo);

	pthread_mutex_unlock(&bufmgr->lock);

	return bo;
}

void
tbm_bo_unref(tbm_bo bo)
{
	TBM_RETURN_IF_FAIL(_tbm_bo_is_valid(bo));

	tbm_bufmgr bufmgr = bo->bufmgr;

	pthread_mutex_lock(&bufmgr->lock);

	_tbm_bo_unref(bo);

	pthread_mutex_unlock(&bufmgr->lock);
}

tbm_bo
tbm_bo_alloc(tbm_bufmgr bufmgr, int size, int flags)
{
	TBM_RETURN_VAL_IF_FAIL(TBM_BUFMGR_IS_VALID(bufmgr) && (size > 0), NULL);

	tbm_bo bo = NULL;
	void *bo_priv = NULL;

	bo = calloc(1, sizeof(struct _tbm_bo));
	if (!bo) {
		_tbm_set_last_result(TBM_BO_ERROR_HEAP_ALLOC_FAILED);
		return NULL;
	}

	bo->bufmgr = bufmgr;

	pthread_mutex_lock(&bufmgr->lock);

	bo_priv = bufmgr->backend->bo_alloc(bo, size, flags);
	if (!bo_priv) {
		_tbm_set_last_result(TBM_BO_ERROR_BO_ALLOC_FAILED);
		free(bo);
		pthread_mutex_unlock(&bufmgr->lock);
		return NULL;
	}

	bo->ref_cnt = 1;
	bo->flags = flags;
	bo->priv = bo_priv;

	LIST_INITHEAD(&bo->user_data_list);

	LIST_ADD(&bo->item_link, &bufmgr->bo_list);

	pthread_mutex_unlock(&bufmgr->lock);

	return bo;
}

tbm_bo
tbm_bo_import(tbm_bufmgr bufmgr, unsigned int key)
{
	TBM_RETURN_VAL_IF_FAIL(TBM_BUFMGR_IS_VALID(bufmgr), NULL);

	tbm_bo bo = NULL;
	tbm_bo bo2 = NULL;
	tbm_bo tmp = NULL;
	void *bo_priv = NULL;

	pthread_mutex_lock(&bufmgr->lock);

	bo = calloc(1, sizeof(struct _tbm_bo));
	if (!bo) {
		pthread_mutex_unlock(&bufmgr->lock);
		return NULL;
	}

	bo->bufmgr = bufmgr;

	bo_priv = bufmgr->backend->bo_import(bo, key);
	if (!bo_priv) {
		_tbm_set_last_result(TBM_BO_ERROR_IMPORT_FAILED);
		free(bo);
		pthread_mutex_unlock(&bufmgr->lock);
		return NULL;
	}

	if (!LIST_IS_EMPTY(&bufmgr->bo_list)) {
		LIST_FOR_EACH_ENTRY_SAFE(bo2, tmp, &bufmgr->bo_list, item_link) {
			if (bo2->priv == bo_priv) {
				DBG("find bo(%p, ref:%d key:%d) in list\n",
				    bo2, bo2->ref_cnt, key);

				bo2->ref_cnt++;
				free(bo);
				pthread_mutex_unlock(&bufmgr->lock);
				return bo2;
			}
		}
	}

	bo->ref_cnt = 1;
	bo->priv = bo_priv;

	if (bufmgr->backend->bo_get_flags)
		bo->flags = bufmgr->backend->bo_get_flags(bo);
	else
		bo->flags = TBM_BO_DEFAULT;

	LIST_INITHEAD(&bo->user_data_list);

	LIST_ADD(&bo->item_link, &bufmgr->bo_list);

	pthread_mutex_unlock(&bufmgr->lock);

	return bo;
}

tbm_bo
tbm_bo_import_fd(tbm_bufmgr bufmgr, tbm_fd fd)
{
	TBM_RETURN_VAL_IF_FAIL(TBM_BUFMGR_IS_VALID(bufmgr), NULL);

	tbm_bo bo = NULL;
	tbm_bo bo2 = NULL;
	tbm_bo tmp = NULL;
	void *bo_priv = NULL;

	pthread_mutex_lock(&bufmgr->lock);

	bo = calloc(1, sizeof(struct _tbm_bo));
	if (!bo) {
		pthread_mutex_unlock(&bufmgr->lock);
		return NULL;
	}

	bo->bufmgr = bufmgr;

	bo_priv = bufmgr->backend->bo_import_fd(bo, fd);
	if (!bo_priv) {
		_tbm_set_last_result(TBM_BO_ERROR_IMPORT_FD_FAILED);
		free(bo);
		pthread_mutex_unlock(&bufmgr->lock);
		return NULL;
	}

	if (!LIST_IS_EMPTY(&bufmgr->bo_list)) {
		LIST_FOR_EACH_ENTRY_SAFE(bo2, tmp, &bufmgr->bo_list, item_link) {
			if (bo2->priv == bo_priv) {
				DBG("find bo(%p, ref:%d, fd:%d) in list\n",
				    bo2, bo2->ref_cnt, fd);

				bo2->ref_cnt++;
				free(bo);
				pthread_mutex_unlock(&bufmgr->lock);
				return bo2;
			}
		}
	}

	bo->ref_cnt = 1;
	bo->priv = bo_priv;

	if (bufmgr->backend->bo_get_flags)
		bo->flags = bufmgr->backend->bo_get_flags(bo);
	else
		bo->flags = TBM_BO_DEFAULT;

	LIST_INITHEAD(&bo->user_data_list);

	LIST_ADD(&bo->item_link, &bufmgr->bo_list);

	pthread_mutex_unlock(&bufmgr->lock);

	return bo;
}

tbm_key
tbm_bo_export(tbm_bo bo)
{
	TBM_RETURN_VAL_IF_FAIL(_tbm_bo_is_valid(bo), 0);

	tbm_bufmgr bufmgr;
	tbm_key ret;

	bufmgr = bo->bufmgr;

	pthread_mutex_lock(&bufmgr->lock);
	ret = bufmgr->backend->bo_export(bo);
	if (!ret) {
		_tbm_set_last_result(TBM_BO_ERROR_EXPORT_FAILED);
		pthread_mutex_unlock(&bufmgr->lock);
		return ret;
	}
	pthread_mutex_unlock(&bufmgr->lock);

	return ret;
}

tbm_fd
tbm_bo_export_fd(tbm_bo bo)
{
	TBM_RETURN_VAL_IF_FAIL(_tbm_bo_is_valid(bo), -1);

	tbm_bufmgr bufmgr;
	int ret;

	bufmgr = bo->bufmgr;

	pthread_mutex_lock(&bufmgr->lock);
	ret = bufmgr->backend->bo_export_fd(bo);
	if (ret < 0) {
		_tbm_set_last_result(TBM_BO_ERROR_EXPORT_FD_FAILED);
		pthread_mutex_unlock(&bufmgr->lock);
		return ret;
	}
	pthread_mutex_unlock(&bufmgr->lock);

	return ret;
}

tbm_bo_handle
tbm_bo_get_handle(tbm_bo bo, int device)
{
	TBM_RETURN_VAL_IF_FAIL(_tbm_bo_is_valid(bo), (tbm_bo_handle) 0);

	tbm_bufmgr bufmgr;
	tbm_bo_handle bo_handle;

	bufmgr = bo->bufmgr;

	pthread_mutex_lock(&bufmgr->lock);
	bo_handle = bufmgr->backend->bo_get_handle(bo, device);
	if (bo_handle.ptr == NULL) {
		_tbm_set_last_result(TBM_BO_ERROR_GET_HANDLE_FAILED);
		pthread_mutex_unlock(&bufmgr->lock);
		return (tbm_bo_handle) NULL;
	}
	pthread_mutex_unlock(&bufmgr->lock);

	return bo_handle;
}

tbm_bo_handle
tbm_bo_map(tbm_bo bo, int device, int opt)
{
	TBM_RETURN_VAL_IF_FAIL(_tbm_bo_is_valid(bo), (tbm_bo_handle) 0);

	tbm_bufmgr bufmgr;
	tbm_bo_handle bo_handle;

	bufmgr = bo->bufmgr;

	pthread_mutex_lock(&bufmgr->lock);

	if (!_tbm_bo_lock(bo, device, opt)) {
		_tbm_set_last_result(TBM_BO_ERROR_LOCK_FAILED);
		TBM_LOG_E("error fail to lock bo:%p)\n",
			bo);
		pthread_mutex_unlock(&bufmgr->lock);
		return (tbm_bo_handle) NULL;
	}

	bo_handle = bufmgr->backend->bo_map(bo, device, opt);
	if (bo_handle.ptr == NULL) {
		_tbm_set_last_result(TBM_BO_ERROR_MAP_FAILED);
		TBM_LOG_E("error fail to map bo:%p\n",
			bo);

		_tbm_bo_unlock(bo);
		pthread_mutex_unlock(&bufmgr->lock);
		return (tbm_bo_handle) NULL;
	}

	/* increase the map_count */
	bo->map_cnt++;

	pthread_mutex_unlock(&bufmgr->lock);

	return bo_handle;
}

int
tbm_bo_unmap(tbm_bo bo)
{
	TBM_RETURN_VAL_IF_FAIL(_tbm_bo_is_valid(bo), 0);

	tbm_bufmgr bufmgr;
	int ret;

	bufmgr = bo->bufmgr;

	pthread_mutex_lock(&bufmgr->lock);

	ret = bufmgr->backend->bo_unmap(bo);
	if (!ret) {

		_tbm_set_last_result(TBM_BO_ERROR_UNMAP_FAILED);
		pthread_mutex_unlock(&bufmgr->lock);
		return ret;
	}

	/* decrease the map_count */
	bo->map_cnt--;

	_tbm_bo_unlock(bo);

	pthread_mutex_unlock(&bufmgr->lock);

	return ret;
}

int
tbm_bo_swap(tbm_bo bo1, tbm_bo bo2)
{
	TBM_RETURN_VAL_IF_FAIL(_tbm_bo_is_valid(bo1), 0);
	TBM_RETURN_VAL_IF_FAIL(_tbm_bo_is_valid(bo2), 0);

	void *temp;

	pthread_mutex_lock(&bo1->bufmgr->lock);

	if (bo1->bufmgr->backend->bo_size(bo1) != bo2->bufmgr->backend->bo_size(bo2)) {
		_tbm_set_last_result(TBM_BO_ERROR_SWAP_FAILED);
		pthread_mutex_unlock(&bo1->bufmgr->lock);
		return 0;
	}

	temp = bo1->priv;
	bo1->priv = bo2->priv;
	bo2->priv = temp;

	pthread_mutex_unlock(&bo1->bufmgr->lock);

	return 1;
}

int
tbm_bo_locked(tbm_bo bo)
{
	TBM_RETURN_VAL_IF_FAIL(_tbm_bo_is_valid(bo), 0);

	tbm_bufmgr bufmgr;

	bufmgr = bo->bufmgr;

	if (bufmgr->lock_type == LOCK_TRY_NEVER)
		return 0;

	pthread_mutex_lock(&bufmgr->lock);

	if (bo->lock_cnt > 0) {
		pthread_mutex_unlock(&bufmgr->lock);
		return 1;
	}

	pthread_mutex_unlock(&bufmgr->lock);

	return 0;
}

int
tbm_bo_add_user_data(tbm_bo bo, unsigned long key,
		     tbm_data_free data_free_func)
{
	TBM_RETURN_VAL_IF_FAIL(_tbm_bo_is_valid(bo), 0);

	tbm_user_data *data;

	/* check if the data according to the key exist if so, return false. */
	data = user_data_lookup(&bo->user_data_list, key);
	if (data) {
		TBM_LOG_W("waring user data already exist. key:%ld\n",
			key);
		return 0;
	}

	data = user_data_create(key, data_free_func);
	if (!data)
		return 0;

	LIST_ADD(&data->item_link, &bo->user_data_list);

	return 1;
}

int
tbm_bo_set_user_data(tbm_bo bo, unsigned long key, void *data)
{
	TBM_RETURN_VAL_IF_FAIL(_tbm_bo_is_valid(bo), 0);

	tbm_user_data *old_data;

	if (LIST_IS_EMPTY(&bo->user_data_list))
		return 0;

	old_data = user_data_lookup(&bo->user_data_list, key);
	if (!old_data)
		return 0;

	if (old_data->data && old_data->free_func)
		old_data->free_func(old_data->data);

	old_data->data = data;

	return 1;
}

int
tbm_bo_get_user_data(tbm_bo bo, unsigned long key, void **data)
{
	TBM_RETURN_VAL_IF_FAIL(_tbm_bo_is_valid(bo), 0);

	tbm_user_data *old_data;

	if (!data || LIST_IS_EMPTY(&bo->user_data_list))
		return 0;

	old_data = user_data_lookup(&bo->user_data_list, key);
	if (!old_data) {
		*data = NULL;
		return 0;
	}

	*data = old_data->data;

	return 1;
}

int
tbm_bo_delete_user_data(tbm_bo bo, unsigned long key)
{
	TBM_RETURN_VAL_IF_FAIL(_tbm_bo_is_valid(bo), 0);

	tbm_user_data *old_data = (void *)0;

	if (LIST_IS_EMPTY(&bo->user_data_list))
		return 0;

	old_data = user_data_lookup(&bo->user_data_list, key);
	if (!old_data)
		return 0;

	user_data_delete(old_data);

	return 1;
}

unsigned int
tbm_bufmgr_get_capability(tbm_bufmgr bufmgr)
{
	TBM_RETURN_VAL_IF_FAIL(TBM_BUFMGR_IS_VALID(bufmgr), 0);

	unsigned int capability = TBM_BUFMGR_CAPABILITY_NONE;

	if (bufmgr->backend->bo_import && bufmgr->backend->bo_export)
		capability |= TBM_BUFMGR_CAPABILITY_SHARE_KEY;

	if (bufmgr->backend->bo_import_fd && bufmgr->backend->bo_export_fd)
		capability |= TBM_BUFMGR_CAPABILITY_SHARE_FD;

	return capability;
}

int
tbm_bo_get_flags(tbm_bo bo)
{
	TBM_RETURN_VAL_IF_FAIL(_tbm_bo_is_valid(bo), 0);

	return bo->flags;
}

/* LCOV_EXCL_START */
tbm_error_e
tbm_get_last_error(void)
{
	return tbm_last_error;
}

void
tbm_bufmgr_debug_show(tbm_bufmgr bufmgr)
{
	TBM_RETURN_IF_FAIL(bufmgr != NULL);
	tbm_bo bo = NULL, tmp_bo = NULL;
	int bo_cnt = 0;

	tbm_surface_h surf = NULL, tmp_surf = NULL;
	int surf_cnt = 0;
	int i;
	char app_name[255] = {0,};
	unsigned int pid = 0;

	pthread_mutex_lock(&gLock);

	TBM_DEBUG("\n");
	_tbm_util_get_appname_from_pid(getpid(), app_name);
	_tbm_util_get_appname_brief(app_name);
	TBM_DEBUG("============TBM DEBUG: %s(%d)===========================\n",
		  app_name, getpid());
	memset(app_name, 0x0, 255 * sizeof(char));

	TBM_DEBUG("[tbm_surface information]\n");
	TBM_DEBUG("no  surface              refcnt  width  height  bpp  size      num_bos num_planes flags format              app_name\n");
	/* show the tbm_surface information in surf_list */
	if (!LIST_IS_EMPTY(&bufmgr->surf_list)) {
		LIST_FOR_EACH_ENTRY_SAFE(surf, tmp_surf, &bufmgr->surf_list, item_link) {
			pid = _tbm_surface_internal_get_debug_pid(surf);
			if (!pid) {
				/* if pid is null, set the self_pid */
				pid = getpid();
			}

			_tbm_util_get_appname_from_pid(pid, app_name);
			_tbm_util_get_appname_brief(app_name);

			TBM_DEBUG("%-4d%-23p%-6d%-7d%-8d%-5d%-12d%-10d%-9d%-4d%-20s%s\n",
				  ++surf_cnt,
				  surf,
				  surf->refcnt,
				  surf->info.width,
				  surf->info.height,
				  surf->info.bpp,
				  surf->info.size / 1024,
				  surf->num_bos,
				  surf->num_planes,
				  surf->flags,
				  _tbm_surface_internal_format_to_str(surf->info.format),
				  app_name);

			for (i = 0; i < surf->num_bos; i++) {
				TBM_DEBUG(" bo:%-12p  %-26d%-10d\n",
					  surf->bos[i],
					  surf->bos[i]->ref_cnt,
					  tbm_bo_size(surf->bos[i]) / 1024);
			}

			memset(app_name, 0x0, 255 * sizeof(char));
		}
	} else {
		TBM_DEBUG("no tbm_surfaces.\n");
	}
	TBM_DEBUG("\n");

	TBM_DEBUG("[tbm_bo information]\n");
	TBM_DEBUG("no  bo                   refcnt  size     lock_cnt map_cnt flags surface\n");

	/* show the tbm_bo information in bo_list */
	if (!LIST_IS_EMPTY(&bufmgr->bo_list)) {
		LIST_FOR_EACH_ENTRY_SAFE(bo, tmp_bo, &bufmgr->bo_list, item_link) {
			TBM_DEBUG("%-4d%-11p   %-6d%-12d%-9d%-9d%-4d%-11p\n",
				  ++bo_cnt,
				  bo,
				  bo->ref_cnt,
				  tbm_bo_size(bo) / 1024,
				  bo->lock_cnt,
				  bo->map_cnt,
				  bo->flags,
				  bo->surface);
		}
	} else {
		TBM_DEBUG("no tbm_bos.\n");
	}
	TBM_DEBUG("\n");

	TBM_DEBUG("===============================================================\n");

	pthread_mutex_unlock(&gLock);

}

void
tbm_bufmgr_debug_trace(tbm_bufmgr bufmgr, int onoff)
{
	TBM_LOG_D("bufmgr=%p onoff=%d\n", bufmgr, onoff);
	TBM_LOG_D("Not implemented yet.\n");
}

/* internal function */
int
_tbm_bo_set_surface(tbm_bo bo, tbm_surface_h surface)
{
	TBM_RETURN_VAL_IF_FAIL(_tbm_bo_is_valid(bo), 0);

	bo->surface = surface;

	return 1;
}

int
tbm_bufmgr_bind_native_display(tbm_bufmgr bufmgr, void *NativeDisplay)
{
	TBM_RETURN_VAL_IF_FAIL(TBM_BUFMGR_IS_VALID(bufmgr), 0);

	int ret;

	pthread_mutex_lock(&bufmgr->lock);

	if (!bufmgr->backend->bufmgr_bind_native_display) {
		pthread_mutex_unlock(&bufmgr->lock);
		return 1;
	}

	ret = bufmgr->backend->bufmgr_bind_native_display(bufmgr, NativeDisplay);
	if (!ret) {
		pthread_mutex_unlock(&bufmgr->lock);
		return 0;
	}

	pthread_mutex_unlock(&bufmgr->lock);

	return 1;
}
/* LCOV_EXCL_STOP */