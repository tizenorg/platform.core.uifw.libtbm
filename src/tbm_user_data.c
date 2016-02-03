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
#include "tbm_user_data.h"

tbm_user_data *user_data_lookup(struct list_head *user_data_list, unsigned long key)
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

tbm_user_data *user_data_create(unsigned long key, tbm_data_free data_free_func)
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

void user_data_delete(tbm_user_data * user_data)
{
	if (user_data->data && user_data->free_func)
		user_data->free_func(user_data->data);

	LIST_DEL(&user_data->item_link);

	free(user_data);
}

