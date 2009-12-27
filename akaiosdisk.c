/* Akai OS backup format disk reader
 * Author: Phil Ashby
 * Date: October 2009
 */

#define _LARGEFILE64_SOURCE
#include <string.h>
#ifdef WIN32
#include <io.h>
#include <stdlib.h>
#include <malloc.h>
#else
#include <unistd.h>
#include <stdlib.h>
#endif
#include "deepstripper.h"
#include "akaiosdisk.h"
#include "akaiosutil.h"

// Internal constants
#define AOSD_DIRENTSIZE	0x20
#define AOSD_MAXBLOCKS	0x100
#define AOSD_DIRNAMEOFS	0x04
#define AOSD_DIRNAMESIZ	0x10
#define AOSD_DIRPROJOFS	0x1c
#define AOSD_DIRPROJSIZ	0x04
#define AOSD_VOLUMEID	"AKAIOSK1"
#define AOSD_DIRENTID	"DR4."

// Attempt to read an AKAI OS multi-project backup disk
// Returns: 0 on success, filled out disk info
//         -1 on failure, disk info undefined

int akaiosdisk_read(int fd, AkaiOsDisk *disk) {
	char buf[AOSD_BLOCKSIZE];
	unsigned int b, o;

// Look for disk signature, then directory start
	if (read(fd, buf, sizeof(buf))!=sizeof(buf))
		return -1;

	if (memcmp(buf, AOSD_VOLUMEID, 8)!=0)
		return -1;
	_DBG(_DBG_BLK, "aosd: found signature, looking for directory\n");

	for (b=1; b<AOSD_MAXBLOCKS; b++) {
		if (read(fd, buf, sizeof(buf))!=sizeof(buf))
			return -1;
		if (memcmp(buf, AOSD_DIRENTID, 4)==0)
			break;
	}
	if (b>=AOSD_MAXBLOCKS)
		return -1;
	_DBG(_DBG_BLK, "aosd: found directory, reading projects..\n");

// We have a directory, start reading it...
	disk->offset = b-1;
	disk->dirsize = 0;
	disk->dir = NULL;
	for (o=0; b<AOSD_MAXBLOCKS; b++) {
		char name[17];
		unsigned int ofs;
		if (o>=AOSD_BLOCKSIZE) {
			_DBG(_DBG_BLK, "aosd: read directory block\n");
			if (read(fd, buf, sizeof(buf))!=sizeof(buf))
				return -1;
			o=0;
			++b;
		}
		memcpy(name, buf+o+AOSD_DIRNAMEOFS, AOSD_DIRNAMESIZ);
		name[16]=0;
		memcpy(&ofs, buf+o+AOSD_DIRPROJOFS, AOSD_DIRPROJSIZ);
		ofs=be2hl(ofs);
		if (!name[0])	// End of dir marked by empty name
			break;
		AkaiOsDisk_Dirent *e = (AkaiOsDisk_Dirent *)malloc(sizeof(*e));
		if (!e)
			return -1;
		strcpy(e->name, name);
		e->offset = ofs;
		e->next = disk->dir;
		disk->dir = e;
		disk->dirsize++;
		o+=AOSD_DIRENTSIZE;
		_DBG(_DBG_BLK, "aosd: project: %s @ 0x%08x\n", e->name, e->offset);
	}
	return 0;
}


// Clear memory blocks in directory, and clear disk structure fields

void akaiosdisk_clear(AkaiOsDisk *disk) {
	while (disk->dir) {
		AkaiOsDisk_Dirent *e = disk->dir;
		disk->dir = e->next;
		free(e);
	}
	disk->dirsize = 0;
	disk->offset = 0;
}

// Calculate 64-bit disk offset for specified project.
// Returns: offset on success, 0 on failure (use perror or strerror)

off64_t akaiosdisk_project(AkaiOsDisk *disk, char *name) {
	AkaiOsDisk_Dirent *e = disk->dir;
	while (e && strcmp(name, e->name)!=0)
		e = e->next;
	if (e)
		return ((off64_t)e->offset * (off64_t)AOSD_BLOCKSIZE);
	return 0;
}
