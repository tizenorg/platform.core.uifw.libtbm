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

typedef struct
{
    struct list_head head;

    struct list_head tail;

    int count;
} queue;

typedef struct
{
    tbm_surface_h surface;

    struct list_head item_link;
} queue_node;

struct _tbm_surface_queue
{
    int width;
    int height;
    int format;
    int size;
    int flags;

    queue free_queue;
    queue duty_queue;
    queue_node **node_list;

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
    pthread_cond_t duty_cond;
};

static queue_node *
_queue_node_create (void)
{
    queue_node *node = (queue_node *) calloc (1, sizeof(queue_node));
    TBM_RETURN_VAL_IF_FAIL (node != NULL, NULL);

    return node;
}

static void
_queue_node_delete (queue_node *node)
{
    if (node->surface)
        tbm_surface_destroy (node->surface);
    LIST_DEL (&node->item_link);
    free (node);
}

static int
_queue_is_empty (queue *queue)
{
    if (queue->head.next == &queue->tail)
        return 1;

    return 0;
}

static void
_queue_node_push_back (queue *queue, queue_node *node)
{
    LIST_ADDTAIL (&node->item_link, &queue->tail);
    queue->count++;
    return;
}

static void
_queue_node_push_front (queue *queue, queue_node *node)
{
    LIST_ADD (&node->item_link, &queue->head);
    queue->count++;
    return;
}

static queue_node *
_queue_node_pop_front (queue *queue)
{
    queue_node *node = NULL;

    node = LIST_ENTRY (queue_node, queue->head.next, item_link);

    LIST_DEL (&node->item_link);
    queue->count--;

    return node;
}

static int
_queue_node_exist_in_queue (queue *queue, queue_node *node)
{
    queue_node *search_node = NULL;
    queue_node *temp = NULL;

    if (!_queue_is_empty(queue))
    {
        LIST_FOR_EACH_ENTRY_SAFE (search_node, temp, &queue->head, item_link)
        {
            if (search_node == node)
                return 1;
        }
    }

    return 0;
}

static void
_queue_init (queue *queue)
{
    LIST_INITHEAD (&queue->head);
    LIST_INITHEAD (&queue->tail);
    LIST_ADDTAIL (&queue->head, &queue->tail);
    queue->count = 0;
}

tbm_surface_queue_error_e
tbm_surface_queue_enqueue (tbm_surface_queue_h surface_queue, tbm_surface_h surface)
{
    TBM_RETURN_VAL_IF_FAIL (surface_queue != NULL, TBM_SURFACE_QUEUE_ERROR_INVALID_QUEUE);
    TBM_RETURN_VAL_IF_FAIL (surface_queue != NULL, TBM_SURFACE_QUEUE_ERROR_INVALID_SURFACE);

    int i;

    pthread_mutex_lock (&surface_queue->lock);

    for (i = 0 ; i < surface_queue->size ; i++)
    {
        if (surface_queue->node_list[i]->surface == surface)
            break;
    }

    if (i == surface_queue->size)
    {
        TBM_LOG ("Can't find the surface in queue\n");
        pthread_mutex_unlock (&surface_queue->lock);
        return TBM_SURFACE_QUEUE_ERROR_INVALID_SURFACE;
    }

    if (_queue_node_exist_in_queue (&surface_queue->duty_queue, surface_queue->node_list[i]))
    {
        TBM_LOG ("Surface exist in queue\n");
        pthread_mutex_unlock (&surface_queue->lock);
        return TBM_SURFACE_QUEUE_ERROR_INVALID_SURFACE;
    }

    _queue_node_push_back(&surface_queue->duty_queue, surface_queue->node_list[i]);

    pthread_mutex_unlock (&surface_queue->lock);
    pthread_cond_signal(&surface_queue->duty_cond);

    if (surface_queue->acquirable_cb)
        surface_queue->acquirable_cb(surface_queue, surface_queue->acquirable_cb_data);

    return TBM_SURFACE_QUEUE_ERROR_NONE;
}

tbm_surface_queue_error_e
tbm_surface_queue_dequeue (tbm_surface_queue_h surface_queue, tbm_surface_h *surface)
{
    TBM_RETURN_VAL_IF_FAIL (surface_queue != NULL, TBM_SURFACE_QUEUE_ERROR_INVALID_QUEUE);
    TBM_RETURN_VAL_IF_FAIL (surface_queue->free_queue.count > 0, TBM_SURFACE_QUEUE_ERROR_EMPTY);

    pthread_mutex_lock (&surface_queue->lock);

    if (_queue_is_empty (&surface_queue->free_queue))
    {
        TBM_LOG ("Surface queue is empty\n");
        pthread_mutex_unlock (&surface_queue->lock);
        return TBM_SURFACE_QUEUE_ERROR_EMPTY;
    }

    queue_node *node = NULL;

    node = _queue_node_pop_front (&surface_queue->free_queue);
    if (node == NULL)
    {
        TBM_LOG ("_queue_node_pop_front is failed\n");
        pthread_mutex_unlock (&surface_queue->lock);
        return TBM_SURFACE_QUEUE_ERROR_INVALID_QUEUE;
    }

	if (!node->surface)
    {
        tbm_surface_h surface = tbm_surface_internal_create_with_flags (surface_queue->width,
                                                    surface_queue->height,
                                                    surface_queue->format,
                                                    surface_queue->flags);
        if (surface == NULL)
        {
            TBM_LOG ("tbm surface create  failed");
	        pthread_mutex_unlock (&surface_queue->lock);
	        return TBM_SURFACE_QUEUE_ERROR_SURFACE_ALLOC_FAILED;
        }
        node->surface = surface;
    }

    *surface = node->surface;

    pthread_mutex_unlock (&surface_queue->lock);

    return TBM_SURFACE_QUEUE_ERROR_NONE;
}

tbm_surface_queue_error_e
tbm_surface_queue_release (tbm_surface_queue_h surface_queue, tbm_surface_h surface)
{
    TBM_RETURN_VAL_IF_FAIL (surface_queue != NULL, TBM_SURFACE_QUEUE_ERROR_INVALID_QUEUE);
    TBM_RETURN_VAL_IF_FAIL (surface != NULL, TBM_SURFACE_QUEUE_ERROR_INVALID_SURFACE);

    pthread_mutex_lock (&surface_queue->lock);

    int i;
    for (i = 0 ; i < surface_queue->size ; i++)
    {
        if (surface_queue->node_list[i]->surface == surface)
            break;
    }
    if (i == surface_queue->size)
    {
        TBM_LOG ("Can't find the surface in queue\n");
        pthread_mutex_unlock (&surface_queue->lock);
        return TBM_SURFACE_QUEUE_ERROR_INVALID_SURFACE;
    }

    if (_queue_node_exist_in_queue (&surface_queue->free_queue, surface_queue->node_list[i]))
    {
        TBM_LOG ("Surface exist in queue\n");
        pthread_mutex_unlock (&surface_queue->lock);
        return TBM_SURFACE_QUEUE_ERROR_INVALID_SURFACE;
    }

    _queue_node_push_front(&surface_queue->free_queue, surface_queue->node_list[i]);

    pthread_mutex_unlock (&surface_queue->lock);
    pthread_cond_signal(&surface_queue->free_cond);

    if (surface_queue->dequeuable_cb)
        surface_queue->dequeuable_cb (surface_queue, surface_queue->dequeuable_cb_data);

    return TBM_SURFACE_QUEUE_ERROR_NONE;
}

tbm_surface_queue_error_e
tbm_surface_queue_acquire (tbm_surface_queue_h surface_queue, tbm_surface_h *surface)
{
    TBM_RETURN_VAL_IF_FAIL (surface_queue != NULL, TBM_SURFACE_QUEUE_ERROR_INVALID_QUEUE);
    TBM_RETURN_VAL_IF_FAIL (surface_queue->duty_queue.count > 0, TBM_SURFACE_QUEUE_ERROR_EMPTY);

    pthread_mutex_lock (&surface_queue->lock);

    if (_queue_is_empty (&surface_queue->duty_queue))
    {
        TBM_LOG ("Surface queue is empty\n");
        pthread_mutex_unlock (&surface_queue->lock);
        return TBM_SURFACE_QUEUE_ERROR_EMPTY;
    }

    queue_node *node = NULL;

    node = _queue_node_pop_front (&surface_queue->duty_queue);
    if (node == NULL)
    {
        TBM_LOG ("_queue_node_pop_front  failed\n");
        pthread_mutex_unlock (&surface_queue->lock);
        return TBM_SURFACE_QUEUE_ERROR_INVALID_QUEUE;
    }

    *surface = node->surface;

    pthread_mutex_unlock (&surface_queue->lock);

    return TBM_SURFACE_QUEUE_ERROR_NONE;
}

int
tbm_surface_queue_can_dequeue (tbm_surface_queue_h surface_queue, int wait)
{
    TBM_RETURN_VAL_IF_FAIL (surface_queue != NULL, 0);

    pthread_mutex_lock (&surface_queue->lock);

    if (_queue_is_empty (&surface_queue->free_queue))
    {
        if (wait)
        {
            pthread_cond_wait (&surface_queue->free_cond, &surface_queue->lock);
            pthread_mutex_unlock (&surface_queue->lock);
            return 1;
        }

        pthread_mutex_unlock (&surface_queue->lock);
        return 0;
    }

    pthread_mutex_unlock (&surface_queue->lock);
    return 1;
}

int
tbm_surface_queue_can_acquire (tbm_surface_queue_h surface_queue, int wait)
{
    TBM_RETURN_VAL_IF_FAIL (surface_queue != NULL, 0);

    pthread_mutex_lock (&surface_queue->lock);

    if (_queue_is_empty (&surface_queue->duty_queue))
    {
        if (wait)
        {
            pthread_cond_wait (&surface_queue->duty_cond, &surface_queue->lock);
            pthread_mutex_unlock (&surface_queue->lock);
            return 1;
        }

        pthread_mutex_unlock (&surface_queue->lock);
        return 0;
    }

    pthread_mutex_unlock (&surface_queue->lock);

    return 1;
}

tbm_surface_queue_h
tbm_surface_queue_create(int queue_size, int width, int height, int format, int flags)
{
    TBM_RETURN_VAL_IF_FAIL (queue_size > 0, NULL);
    TBM_RETURN_VAL_IF_FAIL (width > 0, NULL);
    TBM_RETURN_VAL_IF_FAIL (height > 0, NULL);
    TBM_RETURN_VAL_IF_FAIL (format > 0, NULL);

    int i, j;
    tbm_surface_queue_h surface_queue = (tbm_surface_queue_h)calloc(1, sizeof(struct _tbm_surface_queue));
    TBM_RETURN_VAL_IF_FAIL (surface_queue != NULL, NULL);

    pthread_mutex_init (&surface_queue->lock, NULL);
    pthread_cond_init (&surface_queue->free_cond, NULL);
    pthread_cond_init (&surface_queue->duty_cond, NULL);

    surface_queue->width = width;
    surface_queue->height = height;
    surface_queue->format = format;
    surface_queue->flags = flags;
    surface_queue->size = queue_size;
    surface_queue->node_list = (queue_node **)calloc(queue_size, sizeof(queue_node *));
    if (!surface_queue->node_list)
    {
        TBM_LOG ("surface node list alloc failed");
        free (surface_queue);
        pthread_mutex_destroy (&surface_queue->lock);
        return NULL;
    }

	_queue_init (&surface_queue->free_queue);
	_queue_init (&surface_queue->duty_queue);

    for (i = 0 ; i < queue_size; i++)
    {
        queue_node *node = _queue_node_create();
        if (node == NULL)
        {
            TBM_LOG ("surface node create failed");
            goto fail;
        }

        surface_queue->node_list[i] = node;
        _queue_node_push_back (&surface_queue->free_queue, node);
    }

    return surface_queue;

fail:
    for (j = 0 ; j < i ; j++)
        _queue_node_delete (surface_queue->node_list[j]);

    free (surface_queue->node_list);
    free (surface_queue);
    pthread_mutex_destroy (&surface_queue->lock);

    return NULL;
}

void
tbm_surface_queue_destroy (tbm_surface_queue_h surface_queue)
{
    TBM_RETURN_IF_FAIL (surface_queue != NULL);

    if (surface_queue->destroy_cb)
        surface_queue->destroy_cb (surface_queue, surface_queue->destroy_cb_data);

    int i;

    for (i = 0 ; i < surface_queue->size ; i++)
        _queue_node_delete (surface_queue->node_list[i]);

    free (surface_queue->node_list);
    free (surface_queue);
}

tbm_surface_queue_error_e
tbm_surface_queue_set_destroy_cb (tbm_surface_queue_h surface_queue, tbm_surface_queue_notify_cb destroy_cb, void *data)
{
    TBM_RETURN_VAL_IF_FAIL (surface_queue != NULL, TBM_SURFACE_QUEUE_ERROR_INVALID_QUEUE);

    pthread_mutex_lock (&surface_queue->lock);

    surface_queue->destroy_cb = destroy_cb;
    surface_queue->destroy_cb_data = data;

    pthread_mutex_unlock (&surface_queue->lock);

    return TBM_SURFACE_QUEUE_ERROR_NONE;
}

tbm_surface_queue_error_e
tbm_surface_queue_set_dequeuable_cb (tbm_surface_queue_h surface_queue, tbm_surface_queue_notify_cb dequeuable_cb, void *data)
{
    TBM_RETURN_VAL_IF_FAIL (surface_queue != NULL, TBM_SURFACE_QUEUE_ERROR_INVALID_QUEUE);

    pthread_mutex_lock (&surface_queue->lock);

    surface_queue->dequeuable_cb = dequeuable_cb;
    surface_queue->dequeuable_cb_data = data;

    pthread_mutex_unlock (&surface_queue->lock);

    return TBM_SURFACE_QUEUE_ERROR_NONE;
}

tbm_surface_queue_error_e
tbm_surface_queue_set_acquirable_cb (tbm_surface_queue_h surface_queue, tbm_surface_queue_notify_cb acquirable_cb, void *data)
{
    TBM_RETURN_VAL_IF_FAIL (surface_queue != NULL, TBM_SURFACE_QUEUE_ERROR_INVALID_QUEUE);

    pthread_mutex_lock (&surface_queue->lock);

    surface_queue->acquirable_cb = acquirable_cb;
    surface_queue->acquirable_cb_data = data;

    pthread_mutex_unlock (&surface_queue->lock);

    return TBM_SURFACE_QUEUE_ERROR_NONE;
}

int
tbm_surface_queue_get_queue_size (tbm_surface_queue_h surface_queue)
{
    TBM_RETURN_VAL_IF_FAIL (surface_queue != NULL, 0);

    return surface_queue->size;
}

int
tbm_surface_queue_get_width(tbm_surface_queue_h surface_queue)
{
    TBM_RETURN_VAL_IF_FAIL (surface_queue != NULL, 0);

    return surface_queue->width;
}

int
tbm_surface_queue_get_height(tbm_surface_queue_h surface_queue)
{
    return surface_queue->height;
}

int
tbm_surface_queue_get_format(tbm_surface_queue_h surface_queue)
{
    return surface_queue->format;
}

tbm_surface_queue_error_e
tbm_surface_queue_reset(tbm_surface_queue_h surface_queue, int width, int height, int format)
{
    TBM_RETURN_VAL_IF_FAIL (surface_queue != NULL, TBM_SURFACE_QUEUE_ERROR_INVALID_QUEUE);

    int i;
    queue_node *node = NULL;

    if (width == surface_queue->width &&
        height == surface_queue->height &&
        format == surface_queue->format)
        return TBM_SURFACE_QUEUE_ERROR_NONE;

    pthread_mutex_lock (&surface_queue->lock);

    surface_queue->width = width;
    surface_queue->height = height;
    surface_queue->format = format;

    //Reset queue
    _queue_init(&surface_queue->free_queue);
    _queue_init(&surface_queue->duty_queue);

    //Destory surface and Push to free_queue
    for (i = 0 ; i < surface_queue->size; i++)
    {
        node = surface_queue->node_list[i];
        if (node->surface)
        {
            tbm_surface_destroy(node->surface);
            node->surface = NULL;
        }

        _queue_node_push_back(&surface_queue->free_queue, node);
    }

    pthread_mutex_unlock (&surface_queue->lock);
    pthread_cond_signal(&surface_queue->free_cond);

    if (surface_queue->reset_cb)
        surface_queue->reset_cb (surface_queue, surface_queue->reset_cb_data);

    return TBM_SURFACE_QUEUE_ERROR_NONE;
}

tbm_surface_queue_error_e
tbm_surface_queue_set_reset_cb (tbm_surface_queue_h surface_queue, tbm_surface_queue_notify_cb reset_cb, void *data)
{
    TBM_RETURN_VAL_IF_FAIL (surface_queue != NULL, TBM_SURFACE_QUEUE_ERROR_INVALID_QUEUE);

    pthread_mutex_lock (&surface_queue->lock);

    surface_queue->reset_cb = reset_cb;
    surface_queue->reset_cb_data = data;

    pthread_mutex_unlock (&surface_queue->lock);

    return TBM_SURFACE_QUEUE_ERROR_NONE;
}
