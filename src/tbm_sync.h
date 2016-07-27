#ifndef _TBM_SYNC_H_
#define _TBM_SYNC_H

#include <stdint.h>

typedef struct _tbm_sync_fence_info_data tbm_sync_fence_info_data_s;
typedef struct _tbm_sync_pt_info tbm_sync_pt_info_s;

struct _tbm_sync_fence_info_data {
	uint32_t len;
	char name[32];
	int32_t status;
	uint8_t pt_info[0];
};

struct _tbm_sync_pt_info {
	uint32_t len;
	char obj_name[32];
	char driver_name[32];
	int32_t status;
	uint64_t timestamp_ns;
	uint8_t driver_data[0];
};

int tbm_sync_wait(int fd, int timeout);
int tbm_sync_merge(const char *name, int fd1, int fd2);
tbm_sync_fence_info_data_s *sync_fence_info(int fd);
tbm_sync_pt_info_s *tbm_sync_pt_info(tbm_sync_fence_info_data_s *info, tbm_sync_pt_info_s *itr);
void tbm_sync_fence_info_free(tbm_sync_fence_info_data_s *info);
int tbm_sync_timeline_create(void);
int tbm_sync_timeline_inc(int fd, unsigned count);
int tbm_sync_fence_create(int fd, const char *name, unsigned value);

#endif /* _TBM_SYNC_H */
