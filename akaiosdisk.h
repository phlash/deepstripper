/* In memory structure of Akai OS multi-project backup disk */

#ifndef _AKAIOSDISK_H
#define _AKAIOSDISK_H

#define AOSD_BLOCKSIZE	0x1000	// raw disk block size

typedef struct _aosd_dirent {
	char name[20];				// project name (null terminated)
	unsigned int offset;		// project disk block offset
	struct _aosd_dirent *next;	// link to next entry or NULL
} AkaiOsDisk_Dirent;

typedef struct {
	unsigned int offset;		// disk block offset to directory
	int dirsize;				// number of directory entries
	AkaiOsDisk_Dirent *dir;		// link list of directory entries
} AkaiOsDisk;

int akaiosdisk_read(int fd, AkaiOsDisk *disk);
void akaiosdisk_clear(AkaiOsDisk *disk);
AkaiOsDisk_Dirent *akaiosdisk_project_byname(AkaiOsDisk *disk, char *name);
off64_t akaiosdisk_project_bydent(AkaiOsDisk_Dirent *dent);

#endif
