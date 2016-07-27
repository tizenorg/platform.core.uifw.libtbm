#include <fcntl.h>
#include <malloc.h>
#include <stdint.h>
#include <string.h>

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <linux/ioctl.h>
#include <linux/types.h>

#include "tbm_sync.h"

/**
 * struct sync_merge_data - data passed to merge ioctl
 * @fd2:	file descriptor of second fence
 * @name:	name of new fence
 * @fence:	returns the fd of the new fence to userspace
 */
struct sync_merge_data {
	__s32	fd2; /* fd of second fence */
	char	name[32]; /* name of new fence */
	__s32	fence; /* fd on newly created fence */
};

/**
 * struct sync_pt_info - detailed sync_pt information
 * @len:		length of sync_pt_info including any driver_data
 * @obj_name:		name of parent sync_timeline
 * @driver_name:	name of driver implementing the parent
 * @status:		status of the sync_pt 0:active 1:signaled <0:error
 * @timestamp_ns:	timestamp of status change in nanoseconds
 * @driver_data:	any driver dependent data
 */
struct sync_pt_info {
	__u32	len;
	char	obj_name[32];
	char	driver_name[32];
	__s32	status;
	__u64	timestamp_ns;

	__u8	driver_data[0];
};

/**
 * struct sync_fence_info_data - data returned from fence info ioctl
 * @len:	ioctl caller writes the size of the buffer its passing in.
 *		ioctl returns length of sync_fence_data returned to userspace
 *		including pt_info.
 * @name:	name of fence
 * @status:	status of fence. 1: signaled 0:active <0:error
 * @pt_info:	a sync_pt_info struct for every sync_pt in the fence
 */
struct sync_fence_info_data {
	__u32	len;
	char	name[32];
	__s32	status;

	__u8	pt_info[0];
};

#define SYNC_IOC_MAGIC		'>'

/**
 * DOC: SYNC_IOC_WAIT - wait for a fence to signal
 *
 * pass timeout in milliseconds.  Waits indefinitely timeout < 0.
 */
#define SYNC_IOC_WAIT		_IOW(SYNC_IOC_MAGIC, 0, __s32)

/**
 * DOC: SYNC_IOC_MERGE - merge two fences
 *
 * Takes a struct sync_merge_data.  Creates a new fence containing copies of
 * the sync_pts in both the calling fd and sync_merge_data.fd2.  Returns the
 * new fence's fd in sync_merge_data.fence
 */
#define SYNC_IOC_MERGE		_IOWR(SYNC_IOC_MAGIC, 1, struct sync_merge_data)

/**
 * DOC: SYNC_IOC_FENCE_INFO - get detailed information on a fence
 *
 * Takes a struct sync_fence_info_data with extra space allocated for pt_info.
 * Caller should write the size of the buffer into len.  On return, len is
 * updated to reflect the total size of the sync_fence_info_data including
 * pt_info.
 *
 * pt_info is a buffer containing sync_pt_infos for every sync_pt in the fence.
 * To iterate over the sync_pt_infos, use the sync_pt_info.len field.
 */
#define SYNC_IOC_FENCE_INFO	_IOWR(SYNC_IOC_MAGIC, 2,\
	struct sync_fence_info_data)

struct sw_sync_create_fence_data {
	__u32	value;
	char	name[32];
	__s32	fence; /* fd of new fence */
};

#define SW_SYNC_IOC_MAGIC	'W'

#define SW_SYNC_IOC_CREATE_FENCE	_IOWR(SW_SYNC_IOC_MAGIC, 0,\
		struct sw_sync_create_fence_data)
#define SW_SYNC_IOC_INC			_IOW(SW_SYNC_IOC_MAGIC, 1, __u32)

/* Implementation of strlcpy() for platforms that don't already have it. */

/*
 * Copy src to string dst of size siz.  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz == 0).
 * Returns strlen(src); if retval >= siz, truncation occurred.
 */
size_t
strlcpy(char *dst, const char *src, size_t siz)
{
	char *d = dst;
	const char *s = src;
	size_t n = siz;

	/* Copy as many bytes as will fit */
	if (n != 0) {
		while (--n != 0) {
			if ((*d++ = *s++) == '\0')
				break;
		}
	}

	/* Not enough room in dst, add NUL and traverse rest of src */
	if (n == 0) {
		if (siz != 0)
			*d = '\0';		/* NUL-terminate dst */
		while (*s++)
			;
	}

	return(s - src - 1);	/* count does not include NUL */
}

int
tbm_sync_wait(int fd, int timeout)
{
	__s32 to = timeout;

	return ioctl(fd, SYNC_IOC_WAIT, &to);
}

int
tbm_sync_merge(const char *name, int fd1, int fd2)
{
	struct sync_merge_data data;
	int err;

	data.fd2 = fd2;
	strlcpy(data.name, name, sizeof(data.name));

	err = ioctl(fd1, SYNC_IOC_MERGE, &data);
	if (err < 0)
		return err;

	return data.fence;
}

tbm_sync_fence_info_data_s *
tbm_sync_fence_info(int fd)
{
	tbm_sync_fence_info_data_s *info;
	int err;

	info = malloc(4096);
	if (info == NULL)
		return NULL;

	info->len = 4096;
	err = ioctl(fd, SYNC_IOC_FENCE_INFO, info);
	if (err < 0) {
		free(info);
		return NULL;
	}

	return info;
}

tbm_sync_pt_info_s *
tbm_sync_pt_info(tbm_sync_fence_info_data_s *info, tbm_sync_pt_info_s *itr)
{
	if (itr == NULL)
		itr = (tbm_sync_pt_info_s *) info->pt_info;
	else
		itr = (tbm_sync_pt_info_s *) ((__u8 *)itr + itr->len);

	if ((__u8 *)itr - (__u8 *)info >= (int)info->len)
		return NULL;

	return itr;
}

void
tbm_sync_fence_info_free(tbm_sync_fence_info_data_s *info)
{
	free(info);
}


int
tbm_sync_timeline_create(void)
{
	return open("/dev/sw_sync", O_RDWR);
}

int
tbm_sync_timeline_inc(int fd, unsigned count)
{
	__u32 arg = count;

	return ioctl(fd, SW_SYNC_IOC_INC, &arg);
}

int
tbm_sync_fence_create(int fd, const char *name, unsigned value)
{
	struct sw_sync_create_fence_data data;
	int err;

	data.value = value;
	strlcpy(data.name, name, sizeof(data.name));

	err = ioctl(fd, SW_SYNC_IOC_CREATE_FENCE, &data);
	if (err < 0)
		return err;

	return data.fence;
}
