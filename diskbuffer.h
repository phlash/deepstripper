// Block-sized IO buffer to allow use with raw disk drivers that insist on block sized requests
// (yes Windows sucks as usual!)

#ifndef _DISKBUFFER_H
#define _DISKBUFFER_H

#define _LARGEFILE64_SOURCE
#include <unistd.h>

extern ssize_t read_buffered(int fd, void *buf, size_t len);
extern void clear_buffer();

#endif

