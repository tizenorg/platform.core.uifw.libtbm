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

#include "tbm_bufmgr_int.h"
#include "list.h"

#define FREE_QUEUE	1
#define DUTY_QUEUE	2
#define NODE_LIST	4

#define DEBUG 0

#if DEBUG
#define TBM_TRACE() TBM_LOG("[TRACE] %s:%d surface:%p\n", __FUNCTION__, __LINE__, surface_queue)
#define TBM_LOCK() TBM_LOG("[LOCK] %s:%d surface:%p\n", __FUNCTION__, __LINE__, surface_queue)
#define TBM_UNLOCK() TBM_LOG("[UNLOCK] %s:%d surface:%p\n", __FUNCTION__, __LINE__, surface_queue)
#else
#define TBM_TRACE()
#define TBM_LOCK()
#define TBM_UNLOCK()
#endif

typedef struct {
	struct list_head head;
	struct list_head tail;

	int count;
} queue;

typedef struct {
	tbm_surface_h surface;

	struct list_head item_link;
	struct list_head link;

	unsigned int priv_flags;	/*for each queue*/
} queue_node;

typedef struct _tbm_surface_queue_interface {
	void (*init)(tbm_surface_queue_h queue);
	void (*reset)(tbm_surface_queue_h queue);
	void (*destroy)(tbm_surface_queue_h queue);
	void (*need_attach)(tbm_surface_queue_h queue);

	void (*enqueue)(tbm_surface_queue_h queue, queue_node* node);
	void (*release)(tbm_surface_queue_h queue, queue_node* node);
	queue_node* (*dequeue)(tbm_surface_queue_h queue);
	queue_node* (*acquire)(tbm_surface_queue_h queue);
}tbm_surface_queue_interface;

struct _tbm_surface_queue {
	int width;
	int height;
	int format;

	queue free_queue;
	queue dirty_queue;
	struct list_head list;

	tbm_surface_queue_notify_cb destroy_cb;
	void *destroy_cb_data;

	tbm_surface_queue_notify_cb dequeuable_cb;
	void *dequeuable_cb_data;

	tbm_surface_queue_notify_cb acquirable_cb;
	void *acquirable_cb_data;

	tbm_surface_queue_notify_cb reset_cb;
	void *reset_cb_data;

	pthread_mutex_t lock;
	pthread_cond_t free_cond;
	pthread_cond_t dirty_cond;

	const tbm_surface_queue_interface *impl;
	void *impl_data;
};

static queue_node *_queue_node_create(void)
{
	queue_node *node = (queue_node *) calloc(1, sizeof(queue_node));
	TBM_RETURN_VAL_IF_FAIL(node != NULL, NULL);

	return node;
}

static void _queue_node_delete(queue_node * node)
{
	if (node->surface)
		tbm_surface_destroy(node->surface);

	LIST_DEL(&node->item_link);
	LIST_DEL(&node->link);
	free(node);
}

static int _queue_is_empty(queue * queue)
{
	if (queue->head.next == &queue->tail)
		return 1;

	return 0;
}

static void _queue_node_push_back(queue * queue, queue_node * node)
{
	LIST_ADDTAIL(&node->item_link, &queue->tail);
	queue->count++;
	return;
}

static void _queue_node_push_front(queue * queue, queue_node * node)
{
	LIST_ADD(&node->item_link, &queue->head);
	queue->count++;
	return;
}

static queue_node *_queue_node_pop_front(queue * queue)
{
	queue_node *node = NULL;

	node = LIST_ENTRY(queue_node, queue->head.next, item_link);

	LIST_DEL(&node->item_link);
	queue->count--;

	return node;
}

static queue_node *_queue_node_pop(queue * queue, queue_node* node)
{
	LIST_DEL(&node->item_link);
	queue->count--;

	return node;
}

static queue_node* _queue_get_node(tbm_surface_queue_h surface_queue, int type, tbm_surface_h surface, int *out_type)
{
	queue_node *node = NULL;
	queue_node *tmp = NULL;

	if (type == 0)
		type = FREE_QUEUE | DUTY_QUEUE | NODE_LIST;
	if (out_type)
		*out_type = 0;

	if (type & FREE_QUEUE)
	{
		LIST_FOR_EACH_ENTRY_SAFE(node, tmp, &surface_queue->free_queue.head, item_link) {
			if (node->surface == surface)
			{
				if (out_type) *out_type = FREE_QUEUE;
				return node;
			}
		}
	}

	if (type & DUTY_QUEUE)
	{
		LIST_FOR_EACH_ENTRY_SAFE(node, tmp, &surface_queue->dirty_queue.head, item_link) {
			if (node->surface == surface)
			{
				if (out_type) *out_type = DUTY_QUEUE;
				return node;
			}
		}
	}

	if (type & NODE_LIST)
	{
		LIST_FOR_EACH_ENTRY_SAFE(node, tmp, &surface_queue->list, link) {
			if (node->surface == surface)
			{
				if (out_type) *out_type = NODE_LIST;
				return node;
			}
		}
	}

	return NULL;
}

static void _queue_init(queue * queue)
{
	LIST_INITHEAD(&queue->head);
	LIST_INITHEAD(&queue->tail);
	LIST_ADDTAIL(&queue->head, &queue->tail);
	queue->count = 0;
}

void _tbm_surface_queue_attach(tbm_surface_queue_h surface_queue, tbm_surface_h surface)
{
	queue_node *node = NULL;

	node = _queue_node_create();
	TBM_RETURN_IF_FAIL(node != NULL);

	tbm_surface_internal_ref(surface);
	node->surface = surface;

	LIST_ADDTAIL(&node->link, &surface_queue->list);
	_queue_node_push_back(&surface_queue->free_queue, node);
}

void _tbm_surface_queue_detach(tbm_surface_queue_h surface_queue, tbm_surface_h surface)
{
	queue_node *node = NULL;
	int queue_type;

	node = _queue_get_node(surface_queue, 0, surface, &queue_type);
	if (node)
		_queue_node_delete(node);
}

void _tbm_surface_queue_enqueue(tbm_surface_queue_h surface_queue, queue_node* node, int push_back)
{
	if (push_back)
		_queue_node_push_back(&surface_queue->dirty_queue, node);
	else
		_queue_node_push_front(&surface_queue->dirty_queue, node);
}

queue_node* _tbm_surface_queue_dequeue(tbm_surface_queue_h surface_queue)
{
	queue_node *node = NULL;

	if (_queue_is_empty(&surface_queue->free_queue)) {
		if (surface_queue->impl && surface_queue->impl->need_attach)
			surface_queue->impl->need_attach(surface_queue);

		if (_queue_is_empty(&surface_queue->free_queue)) {
			return NULL;
		}
	}

	node = _queue_node_pop_front(&surface_queue->free_queue);

	return node;
}

queue_node* _tbm_surface_queue_acquire(tbm_surface_queue_h surface_queue)
{
	queue_node *node = NULL;

	if (_queue_is_empty(&surface_queue->dirty_queue)) {
		return NULL;
	}

	node = _queue_node_pop_front(&surface_queue->dirty_queue);

	return node;
}

void _tbm_surface_queue_release(tbm_surface_queue_h surface_queue, queue_node* node, int push_back)
{
	if (push_back)
		_queue_node_push_back(&surface_queue->free_queue, node);
	else
		_queue_node_push_front(&surface_queue->free_queue, node);
}

void _tbm_surface_queue_init(tbm_surface_queue_h surface_queue,
				int width, int height, int format,
				const tbm_surface_queue_interface* impl, void *data)
{
	TBM_RETURN_IF_FAIL(surface_queue != NULL);
	TBM_RETURN_IF_FAIL(impl != NULL);

	memset(surface_queue, 0x00, sizeof(struct _tbm_surface_queue));

	pthread_mutex_init(&surface_queue->lock, NULL);
	pthread_cond_init(&surface_queue->free_cond, NULL);
	pthread_cond_init(&surface_queue->dirty_cond, NULL);

	surface_queue->width = width;
	surface_queue->height = height;
	surface_queue->format = format;
	surface_queue->impl = impl;
	surface_queue->impl_data = data;

	_queue_init(&surface_queue->free_queue);
	_queue_init(&surface_queue->dirty_queue);
	LIST_INITHEAD(&surface_queue->list);

	if (surface_queue->impl && surface_queue->impl->init)
		surface_queue->impl->init(surface_queue);
}

tbm_surface_queue_error_e tbm_surface_queue_set_destroy_cb(tbm_surface_queue_h surface_queue, tbm_surface_queue_notify_cb destroy_cb, void *data)
{
	TBM_RETURN_VAL_IF_FAIL(surface_queue != NULL, TBM_SURFACE_QUEUE_ERROR_INVALID_QUEUE);

	pthread_mutex_lock(&surface_queue->lock);

	surface_queue->destroy_cb = destroy_cb;
	surface_queue->destroy_cb_data = data;

	pthread_mutex_unlock(&surface_queue->lock);

	return TBM_SURFACE_QUEUE_ERROR_NONE;
}

tbm_surface_queue_error_e tbm_surface_queue_set_dequeuable_cb(tbm_surface_queue_h surface_queue, tbm_surface_queue_notify_cb dequeuable_cb, void *data)
{
	TBM_RETURN_VAL_IF_FAIL(surface_queue != NULL, TBM_SURFACE_QUEUE_ERROR_INVALID_QUEUE);

	pthread_mutex_lock(&surface_queue->lock);

	surface_queue->dequeuable_cb = dequeuable_cb;
	surface_queue->dequeuable_cb_data = data;

	pthread_mutex_unlock(&surface_queue->lock);

	return TBM_SURFACE_QUEUE_ERROR_NONE;
}

tbm_surface_queue_error_e tbm_surface_queue_set_acquirable_cb(tbm_surface_queue_h surface_queue, tbm_surface_queue_notify_cb acquirable_cb, void *data)
{
	TBM_RETURN_VAL_IF_FAIL(surface_queue != NULL, TBM_SURFACE_QUEUE_ERROR_INVALID_QUEUE);

	pthread_mutex_lock(&surface_queue->lock);

	surface_queue->acquirable_cb = acquirable_cb;
	surface_queue->acquirable_cb_data = data;

	pthread_mutex_unlock(&surface_queue->lock);

	return TBM_SURFACE_QUEUE_ERROR_NONE;
}

int tbm_surface_queue_get_width(tbm_surface_queue_h surface_queue)
{
	TBM_RETURN_VAL_IF_FAIL(surface_queue != NULL, 0);

	return surface_queue->width;
}

int tbm_surface_queue_get_height(tbm_surface_queue_h surface_queue)
{
	return surface_queue->height;
}

int tbm_surface_queue_get_format(tbm_surface_queue_h surface_queue)
{
	return surface_queue->format;
}

tbm_surface_queue_error_e tbm_surface_queue_set_reset_cb(tbm_surface_queue_h surface_queue, tbm_surface_queue_notify_cb reset_cb, void *data)
{
	TBM_RETURN_VAL_IF_FAIL(surface_queue != NULL, TBM_SURFACE_QUEUE_ERROR_INVALID_QUEUE);

	pthread_mutex_lock(&surface_queue->lock);

	surface_queue->reset_cb = reset_cb;
	surface_queue->reset_cb_data = data;

	pthread_mutex_unlock(&surface_queue->lock);

	return TBM_SURFACE_QUEUE_ERROR_NONE;
}


tbm_surface_queue_error_e tbm_surface_queue_enqueue(tbm_surface_queue_h surface_queue, tbm_surface_h surface)
{
	queue_node *node = NULL;
	int queue_type;

	TBM_RETURN_VAL_IF_FAIL(surface_queue != NULL, TBM_SURFACE_QUEUE_ERROR_INVALID_QUEUE);
	TBM_RETURN_VAL_IF_FAIL(surface != NULL, TBM_SURFACE_QUEUE_ERROR_INVALID_SURFACE);

	pthread_mutex_lock(&surface_queue->lock);

	node = _queue_get_node(surface_queue, 0, surface, &queue_type);
	if (node == NULL || queue_type != NODE_LIST)
	{
		TBM_LOG("tbm_surface_queue_enqueue::Surface exist in free_queue or dirty_queue node:%p, queue:%d\n", node, queue_type);
		pthread_mutex_unlock(&surface_queue->lock);
		return TBM_SURFACE_QUEUE_ERROR_INVALID_SURFACE;
	}

	if (surface_queue->impl && surface_queue->impl->enqueue)
		surface_queue->impl->enqueue(surface_queue, node);
	else
		_tbm_surface_queue_enqueue(surface_queue, node, 1);

	if (_queue_is_empty(&surface_queue->dirty_queue)) {
		pthread_mutex_unlock(&surface_queue->lock);
		return TBM_SURFACE_QUEUE_ERROR_NONE;
	}

	pthread_mutex_unlock(&surface_queue->lock);
	pthread_cond_signal(&surface_queue->dirty_cond);

	if (surface_queue->acquirable_cb)
		surface_queue->acquirable_cb(surface_queue, surface_queue->acquirable_cb_data);

	return TBM_SURFACE_QUEUE_ERROR_NONE;
}

tbm_surface_queue_error_e tbm_surface_queue_dequeue(tbm_surface_queue_h surface_queue, tbm_surface_h * surface)
{
	queue_node *node = NULL;

	TBM_RETURN_VAL_IF_FAIL(surface_queue != NULL, TBM_SURFACE_QUEUE_ERROR_INVALID_QUEUE);
	TBM_RETURN_VAL_IF_FAIL(surface != NULL, TBM_SURFACE_QUEUE_ERROR_INVALID_SURFACE);

	pthread_mutex_lock(&surface_queue->lock);

	if (surface_queue->impl && surface_queue->impl->dequeue)
		node = surface_queue->impl->dequeue(surface_queue);
	else
		node = _tbm_surface_queue_dequeue(surface_queue);

	if (node == NULL) {
		*surface = NULL;
		pthread_mutex_unlock(&surface_queue->lock);
		return TBM_SURFACE_QUEUE_ERROR_EMPTY;
	}

	if (node->surface == NULL) {
		*surface = NULL;
		TBM_LOG("_queue_node_pop_front  failed\n");
		pthread_mutex_unlock(&surface_queue->lock);
		return TBM_SURFACE_QUEUE_ERROR_INVALID_QUEUE;
	}

	*surface = node->surface;

	pthread_mutex_unlock(&surface_queue->lock);

	return TBM_SURFACE_QUEUE_ERROR_NONE;
}

int tbm_surface_queue_can_dequeue(tbm_surface_queue_h surface_queue, int wait)
{
	TBM_RETURN_VAL_IF_FAIL(surface_queue != NULL, 0);

	pthread_mutex_lock(&surface_queue->lock);

	if (_queue_is_empty(&surface_queue->free_queue)) {
		if (surface_queue->impl && surface_queue->impl->need_attach)
			surface_queue->impl->need_attach(surface_queue);
	}

	if (_queue_is_empty(&surface_queue->free_queue)) {
		if (wait) {
			pthread_cond_wait(&surface_queue->free_cond, &surface_queue->lock);
			pthread_mutex_unlock(&surface_queue->lock);
			return 1;
		}

		pthread_mutex_unlock(&surface_queue->lock);
		return 0;
	}

	pthread_mutex_unlock(&surface_queue->lock);

	return 1;
}

tbm_surface_queue_error_e tbm_surface_queue_release(tbm_surface_queue_h surface_queue, tbm_surface_h surface)
{
	queue_node *node = NULL;
	int queue_type;

	TBM_RETURN_VAL_IF_FAIL(surface_queue != NULL, TBM_SURFACE_QUEUE_ERROR_INVALID_QUEUE);
	TBM_RETURN_VAL_IF_FAIL(surface != NULL, TBM_SURFACE_QUEUE_ERROR_INVALID_SURFACE);

	pthread_mutex_lock(&surface_queue->lock);

	node = _queue_get_node(surface_queue, 0, surface, &queue_type);
	if (node == NULL || queue_type != NODE_LIST)
	{
		TBM_LOG("tbm_surface_queue_release::Surface exist in free_queue or dirty_queue node:%p, queue:%d\n", node, queue_type);
		pthread_mutex_unlock(&surface_queue->lock);
		return TBM_SURFACE_QUEUE_ERROR_INVALID_SURFACE;
	}

	if (surface_queue->impl && surface_queue->impl->release)
		surface_queue->impl->release(surface_queue, node);
	else
		_tbm_surface_queue_release(surface_queue, node, 1);

	if (_queue_is_empty(&surface_queue->free_queue)) {
		pthread_mutex_unlock(&surface_queue->lock);
		return TBM_SURFACE_QUEUE_ERROR_NONE;
	}

	pthread_mutex_unlock(&surface_queue->lock);
	pthread_cond_signal(&surface_queue->free_cond);

	if (surface_queue->dequeuable_cb)
		surface_queue->dequeuable_cb(surface_queue, surface_queue->dequeuable_cb_data);

	return TBM_SURFACE_QUEUE_ERROR_NONE;
}

tbm_surface_queue_error_e tbm_surface_queue_acquire(tbm_surface_queue_h surface_queue, tbm_surface_h * surface)
{
	queue_node *node = NULL;

	TBM_RETURN_VAL_IF_FAIL(surface_queue != NULL, TBM_SURFACE_QUEUE_ERROR_INVALID_QUEUE);
	TBM_RETURN_VAL_IF_FAIL(surface != NULL, TBM_SURFACE_QUEUE_ERROR_INVALID_SURFACE);

	pthread_mutex_lock(&surface_queue->lock);

	if (surface_queue->impl && surface_queue->impl->acquire)
		node = surface_queue->impl->acquire(surface_queue);
	else
		node = _tbm_surface_queue_acquire(surface_queue);

	if (node == NULL) {
		*surface = NULL;
		pthread_mutex_unlock(&surface_queue->lock);
		return TBM_SURFACE_QUEUE_ERROR_EMPTY;
	}

	if (node->surface == NULL) {
		*surface = NULL;
		TBM_LOG("_queue_node_pop_front  failed\n");
		pthread_mutex_unlock(&surface_queue->lock);
		return TBM_SURFACE_QUEUE_ERROR_INVALID_QUEUE;
	}

	*surface = node->surface;

	pthread_mutex_unlock(&surface_queue->lock);

	return TBM_SURFACE_QUEUE_ERROR_NONE;
}

int tbm_surface_queue_can_acquire(tbm_surface_queue_h surface_queue, int wait)
{
	TBM_RETURN_VAL_IF_FAIL(surface_queue != NULL, 0);

	pthread_mutex_lock(&surface_queue->lock);

	if (_queue_is_empty(&surface_queue->dirty_queue)) {
		if (wait) {
			pthread_cond_wait(&surface_queue->dirty_cond, &surface_queue->lock);
			pthread_mutex_unlock(&surface_queue->lock);
			return 1;
		}

		pthread_mutex_unlock(&surface_queue->lock);
		return 0;
	}

	pthread_mutex_unlock(&surface_queue->lock);

	return 1;
}

void tbm_surface_queue_destroy(tbm_surface_queue_h surface_queue)
{
	queue_node *node = NULL, *tmp = NULL;

	TBM_RETURN_IF_FAIL(surface_queue != NULL);

	if (surface_queue->destroy_cb)
		surface_queue->destroy_cb(surface_queue, surface_queue->destroy_cb_data);

	if (surface_queue->impl && surface_queue->impl->destroy)
		surface_queue->impl->destroy(surface_queue);

	LIST_FOR_EACH_ENTRY_SAFE(node, tmp, &surface_queue->list, link) {
		_queue_node_delete(node);
	}

	pthread_mutex_destroy(&surface_queue->lock);
	free(surface_queue);
}

tbm_surface_queue_error_e tbm_surface_queue_reset(tbm_surface_queue_h surface_queue, int width, int height, int format)
{
	TBM_RETURN_VAL_IF_FAIL(surface_queue != NULL, TBM_SURFACE_QUEUE_ERROR_INVALID_QUEUE);

	queue_node *node = NULL, *tmp = NULL;

	if (width == surface_queue->width && height == surface_queue->height && format == surface_queue->format)
		return TBM_SURFACE_QUEUE_ERROR_NONE;

	pthread_mutex_lock(&surface_queue->lock);

	surface_queue->width = width;
	surface_queue->height = height;
	surface_queue->format = format;

	/* Destory surface and Push to free_queue */
	LIST_FOR_EACH_ENTRY_SAFE(node, tmp, &surface_queue->list, link)
	{
		_queue_node_delete(node);
	}

	/* Reset queue */
	_queue_init(&surface_queue->free_queue);
	_queue_init(&surface_queue->dirty_queue);
	LIST_INITHEAD(&surface_queue->list);

	if (surface_queue->impl && surface_queue->impl->reset)
		surface_queue->impl->reset(surface_queue);

	pthread_mutex_unlock(&surface_queue->lock);
	pthread_cond_signal(&surface_queue->free_cond);

	if (surface_queue->reset_cb)
		surface_queue->reset_cb(surface_queue, surface_queue->reset_cb_data);

	return TBM_SURFACE_QUEUE_ERROR_NONE;
}

typedef struct
{
	int queue_size;
	int num_attached;
	int flags;
}tbm_queue_default;

static void __tbm_queue_default_init(tbm_surface_queue_h surface_queue)
{
	tbm_queue_default* data = surface_queue->impl_data;
	data->num_attached = 0;
}

static void __tbm_queue_default_reset(tbm_surface_queue_h surface_queue)
{
	tbm_queue_default* data = surface_queue->impl_data;
	data->num_attached = 0;
}

static void __tbm_queue_default_destroy(tbm_surface_queue_h surface_queue)
{
	free(surface_queue->impl_data);
}

static void __tbm_queue_default_need_attach(tbm_surface_queue_h surface_queue)
{
	tbm_queue_default* data = surface_queue->impl_data;
	tbm_surface_h surface;

	if (data->queue_size == data->num_attached)
		return;

	surface = tbm_surface_internal_create_with_flags(surface_queue->width,
						surface_queue->height,
						surface_queue->format,
						data->flags);
	TBM_RETURN_IF_FAIL(surface != NULL);
	_tbm_surface_queue_attach(surface_queue, surface);
	tbm_surface_internal_unref(surface);
	data->num_attached++;
}

static const tbm_surface_queue_interface tbm_queue_default_impl =
{
	__tbm_queue_default_init,
	__tbm_queue_default_reset,
	__tbm_queue_default_destroy,
	__tbm_queue_default_need_attach,
	NULL, 				/*__tbm_queue_default_enqueue*/
	NULL, 				/*__tbm_queue_default_release*/
	NULL, 				/*__tbm_queue_default_dequeue*/
	NULL,				/*__tbm_queue_default_acquire*/
};

tbm_surface_queue_h tbm_surface_queue_create(int queue_size, int width, int height, int format, int flags)
{
	TBM_RETURN_VAL_IF_FAIL(queue_size > 0, NULL);
	TBM_RETURN_VAL_IF_FAIL(width > 0, NULL);
	TBM_RETURN_VAL_IF_FAIL(height > 0, NULL);
	TBM_RETURN_VAL_IF_FAIL(format > 0, NULL);

	tbm_surface_queue_h surface_queue = (tbm_surface_queue_h) calloc(1, sizeof(struct _tbm_surface_queue));
	TBM_RETURN_VAL_IF_FAIL(surface_queue != NULL, NULL);

	tbm_queue_default *data = (tbm_queue_default *) calloc(1, sizeof(tbm_queue_default));
	if (data == NULL)
	{
		free(surface_queue);
		return NULL;
	}

	data->queue_size = queue_size;
	data->flags = flags;
	_tbm_surface_queue_init(surface_queue,
		width, height, format,
		&tbm_queue_default_impl, data);

	return surface_queue;
}

typedef struct
{
	int queue_size;
	int num_attached;
	int flags;
	queue dequeue_list;
}tbm_queue_sequence;

static void __tbm_queue_sequence_init(tbm_surface_queue_h surface_queue)
{
	tbm_queue_sequence* data = surface_queue->impl_data;

	data->num_attached = 0;
	_queue_init(&data->dequeue_list);
}

static void __tbm_queue_sequence_reset(tbm_surface_queue_h surface_queue)
{
	tbm_queue_sequence* data = surface_queue->impl_data;

	data->num_attached = 0;
	_queue_init(&data->dequeue_list);
}

static void __tbm_queue_sequence_destroy(tbm_surface_queue_h surface_queue)
{
	free(surface_queue->impl_data);
}

static void __tbm_queue_sequence_need_attach(tbm_surface_queue_h surface_queue)
{
	tbm_queue_sequence* data = surface_queue->impl_data;
	tbm_surface_h surface;

	if (data->queue_size == data->num_attached)
		return;

	surface = tbm_surface_internal_create_with_flags(surface_queue->width,
						surface_queue->height,
						surface_queue->format,
						data->flags);
	TBM_RETURN_IF_FAIL(surface != NULL);
	_tbm_surface_queue_attach(surface_queue, surface);
	tbm_surface_internal_unref(surface);
	data->num_attached++;
}

static void __tbm_queue_sequence_enqueue(tbm_surface_queue_h surface_queue, queue_node* node)
{
	tbm_queue_sequence* data = surface_queue->impl_data;
	queue_node *next = NULL;
	queue_node *tmp = NULL;

	node->priv_flags = 0;

	LIST_FOR_EACH_ENTRY_SAFE(next, tmp, &data->dequeue_list.head, item_link) {
		if (next->priv_flags) break;
        _queue_node_pop(&data->dequeue_list, next);
		_tbm_surface_queue_enqueue(surface_queue, next, 1);
	}
}

static queue_node* __tbm_queue_sequence_dequeue(tbm_surface_queue_h surface_queue)
{
	tbm_queue_sequence* data = surface_queue->impl_data;
	queue_node* node = NULL;

	node = _tbm_surface_queue_dequeue(surface_queue);
	if (node)
	{
		_queue_node_push_back(&data->dequeue_list, node);
		node->priv_flags = 1;
	}

	return node;
}

static const tbm_surface_queue_interface tbm_queue_sequence_impl =
{
	__tbm_queue_sequence_init,
	__tbm_queue_sequence_reset,
	__tbm_queue_sequence_destroy,
	__tbm_queue_sequence_need_attach,
	__tbm_queue_sequence_enqueue,
	NULL, 				/*__tbm_queue_sequence_release*/
	__tbm_queue_sequence_dequeue,
	NULL, 				/*__tbm_queue_sequence_acquire*/
};

tbm_surface_queue_h tbm_surface_queue_sequence_create(int queue_size, int width, int height, int format, int flags)
{
	TBM_RETURN_VAL_IF_FAIL(queue_size > 0, NULL);
	TBM_RETURN_VAL_IF_FAIL(width > 0, NULL);
	TBM_RETURN_VAL_IF_FAIL(height > 0, NULL);
	TBM_RETURN_VAL_IF_FAIL(format > 0, NULL);

	tbm_surface_queue_h surface_queue = (tbm_surface_queue_h) calloc(1, sizeof(struct _tbm_surface_queue));
	TBM_RETURN_VAL_IF_FAIL(surface_queue != NULL, NULL);

	tbm_queue_sequence *data = (tbm_queue_sequence *) calloc(1, sizeof(tbm_queue_sequence));
	if (data == NULL)
	{
		free(surface_queue);
		return NULL;
	}

	data->queue_size = queue_size;
	data->flags = flags;
	_tbm_surface_queue_init(surface_queue,
		width, height, format,
		&tbm_queue_sequence_impl, data);

	return surface_queue;
}
