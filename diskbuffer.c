// Block-buffered IO to allow use with raw disk drivers that insist on block sized requests
// (yes Windows sucks as usual!)

#include "diskbuffer.h"
#include "deepstripper.h"
#include "akaiosdisk.h"

static unsigned char g_disk_buffer[AOSD_BLOCKSIZE];
static size_t g_buffer_len = 0, g_buffer_pos = 0;

ssize_t read_buffered(int fd, void *buf, size_t len) {
	unsigned char *p = (unsigned char *)buf;
	size_t o;
	for (o=0; o<len; o++) {
		if (g_buffer_pos>=g_buffer_len) {
			_DBG(_DBG_BLK, "diskbuffer fill: o=%d\n", o);
			g_buffer_len = read(fd, g_disk_buffer, AOSD_BLOCKSIZE);
			if (g_buffer_len<=0)
				return g_buffer_len;
			g_buffer_pos = 0;
		}
		p[o] = g_disk_buffer[g_buffer_pos++];
	}
	return o;
}

void clear_buffer() {
	g_buffer_len = g_buffer_pos = 0;
}

