/* AKAI OS backup reader for individual projects */

#define _LARGEFILE64_SOURCE
#include <string.h>
#ifdef WIN32
#include <io.h>
#include <stdlib.h>
#include <malloc.h>
#else
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#endif
#include "deepstripper.h"
#include "akaiosproject.h"


// Internal constants
#define AOSP_DATA1_SIZE_DPS12	0x2700
#define AOSP_DATA1_SIZE_DPS16	0x1b00
#define AOSP_DATA2_SIZE			0x1800

// data block 1 offsets
#define AOSP_HDR_SIZE_OFFSET	0x1016
#define AOSP_SCN_SIZE_OFFSET	0x102e
#define AOSP_SPL_RATE_OFFSET	0x110c
#define AOSP_VTRK_ASGN_OFFSET	0x1128
#define AOSP_SPL_SIZE_OFFSET	0x1199
#define AOSP_PRJ_NAME_OFFSET	0x12cc
#define AOSP_MEM_IN_OFFSET		0x14c0
#define AOSP_MEM_OUT_OFFSET	0x14c4

// data block 2 offsets
#define AOSP_VTRK_NAME_OFFSET	0x0800

// Convenience macro to calculate next segment data offset from sample count and size
#define aosp_next(proj, cnt, off) {\
	off += (cnt*proj->splbyte); \
	if (off & 0xffff) off = (off&0xffff0000)+0x10000; \
}


// Add a segment to a vtrack
// Returns 0 on success, -1 on failure
static int aosp_add_seg(AkaiOsProject *proj, int vt,
	unsigned int start, unsigned int end, unsigned int off) {
	AkaiOsSegment *seg = (AkaiOsSegment *)malloc(sizeof(AkaiOsSegment));
	if (!seg)
		return -1;
	seg->start = start;
	seg->end = end;
	seg->offset = off + proj->offset;
	seg->next = proj->vtracks[vt].segs;
	proj->vtracks[vt].segs = seg;
	_DBG(_DBG_INT, "aosp: seg added: VT=%d, %08x->%08x @ %08x\n",
		vt, seg->start, seg->end, seg->offset);
	return 0;
}

// Sort track segments
static int aosp_cmp_seg(const void *s1, const void *s2) {
	AkaiOsSegment *p1 = *(AkaiOsSegment **)s1;
	AkaiOsSegment *p2 = *(AkaiOsSegment **)s2;
	return p1->start-p2->start;
}

static int aosp_sort_seg(int vt, AkaiOsVTrack *vtrk) {
	int n, i;
	AkaiOsSegment **tmp;
	AkaiOsSegment *nxt = vtrk->segs;
	// count segments
	for (n=0; nxt; n++) {
		nxt = nxt->next;
	}
	_DBG(_DBG_INT, "aosp: sorting %d segments for track %d..\n", n, vt);
	// special case - one segment is already sorted
	if (1==n)
		return 0;
	// create array of pointers for qsort
	tmp = (AkaiOsSegment **)malloc(n * sizeof(*tmp));
	if (!tmp)
		return -1;
	for (i=0, nxt=vtrk->segs; nxt; i++) {
		tmp[i] = nxt;
		nxt = nxt->next;
	}
	// qsort them
	qsort(tmp, n, sizeof(*tmp), aosp_cmp_seg);
	// push back to ordered chain
	vtrk->segs = NULL;
	for (i=n-1; i>=0; i--) {
		tmp[i]->next = vtrk->segs;
		vtrk->segs = tmp[i];
	}
	free(tmp);
	return 0; 
}

// Read a project header from specified descriptor.
// Returns: 0 on success, -1 on failure

int akaiosproject_read(int fd, int type, AkaiOsProject *proj) {
	char data1[AOSP_DATA1_SIZE_DPS12];
	char data2[AOSP_DATA2_SIZE];
	AkaiOsScene *pScn;
	unsigned short tmp;
	unsigned int ltmp, coff, poff, goff;
	int d1 = AOSP_DPS12==type?AOSP_DATA1_SIZE_DPS12:AOSP_DATA1_SIZE_DPS16;
	int sl = AOSP_DPS12==type?AOSS_DPS12_SIZE:AOSS_DPS16_SIZE;
	int i, sn;

	_DBG(_DBG_BLK, "aosp: reading data1 block..\n");
	if (read(fd, data1, d1)!= d1)
		return -1;
	memcpy(&tmp, &data1[AOSP_HDR_SIZE_OFFSET], sizeof(tmp));
	proj->offset = be2hs(tmp) + 0x1000;
	memcpy(&tmp, &data1[AOSP_SCN_SIZE_OFFSET], sizeof(tmp));
	proj->scnlen = be2hs(tmp);
	switch (data1[AOSP_SPL_RATE_OFFSET]) {
	case 0:
		proj->splrate = 32000;
		break;
	case 1:
		proj->splrate = 44100;
		break;
	case 2:
		proj->splrate = 48000;
		break;
	case 3:
		proj->splrate = 96000;
		break;
	default:
		_DBG(_DBG_BLK, "aosp: invalid sample rate: %d\n", data1[AOSP_SPL_RATE_OFFSET]);
		return -1;
	}
	switch (data1[AOSP_SPL_SIZE_OFFSET]) {
	case 0:
		proj->splsize = 16;
		proj->splbyte = 2;
		break;
	case 1:
		proj->splsize = 24;
		proj->splbyte = 4;
		break;
	default:
		_DBG(_DBG_BLK, "aosp: invalid sample size: %d\n", data1[AOSP_SPL_SIZE_OFFSET]);
		return -1;
	}
	memcpy(proj->name, &data1[AOSP_PRJ_NAME_OFFSET], 16);
	proj->name[16]=0;
	memcpy(&ltmp, &data1[AOSP_MEM_IN_OFFSET], sizeof(ltmp));
	proj->memin = be2hl(ltmp);
	memcpy(&ltmp, &data1[AOSP_MEM_OUT_OFFSET], sizeof(ltmp));
	proj->memout = be2hl(ltmp);
	_DBG(_DBG_PRJ, "aosp: hdr: name=%s rate=%d bits=%d\n",
		proj->name, proj->splrate, proj->splsize);
	_DBG(_DBG_BLK, "aosp: reading default scene..\n");
	if (akaiosscene_read(fd, type, &proj->defscene))
		return -1;
	_DBG(_DBG_BLK, "aosp: reading data2 block..\n");
	if (read(fd, data2, sizeof(data2))!=sizeof(data2))
		return -1;
	if (proj->scnlen) {
		sn = proj->scnlen/sl;
		_DBG(_DBG_BLK, "aosp: reading additional scenes (%d)..\n", sn);
		proj->scenes = malloc(sn * sizeof(AkaiOsScene));
		if (!proj->scenes)
			return -1;
		for (i=0; i<sn; i++) {
			if (akaiosscene_read(fd, type, &proj->scenes[i]))
				return -1;
		}
	}
	_DBG(_DBG_BLK, "aosp: assigning virtual tracks to channels..\n");
	proj->vtracks = (AkaiOsVTrack *)malloc(256 * sizeof(AkaiOsVTrack));
	if (!proj->vtracks)
		return -1;
	for (i=0; i<256; i++) {
		memcpy(proj->vtracks[i].name, &data2[AOSP_VTRK_NAME_OFFSET+i*16], 16);
		proj->vtracks[i].name[16]=0;
		proj->vtracks[i].channel=0;	// 0==un-assigned
		proj->vtracks[i].segs=NULL;
	}
	sn = AOSP_DPS12==type?12:16;
	for (i=0; i<sn; i++) {
		int vt = data1[AOSP_VTRK_ASGN_OFFSET+i] & 0xff;
		proj->vtracks[vt].channel = i+1;	// 1-n==assigned
		_DBG(_DBG_INT, "aosp: vtrack%d->channel%d\n", vt, i+1);
	}
	_DBG(_DBG_BLK, "aosp: reading segment table..\n");
	coff = 0;
	poff = 0;
	goff = 0;
	while (1) {
		char segraw[10];
		unsigned short opvt;
		unsigned int start, end;
		if (read(fd, segraw, sizeof(segraw))!=sizeof(segraw))
			return -1;
		memcpy(&opvt, segraw, sizeof(opvt));
		opvt = be2hs(opvt);
		memcpy(&start, segraw+2, sizeof(start));
		start = be2hl(start);
		memcpy(&end, segraw+6, sizeof(end));
		end = be2hl(end);
		_DBG(_DBG_INT, "aosp: seg read: opvt=%04x start=%08x end=%08x\n", opvt, start, end);
		if (!opvt && !start && !end)
			break;
		switch(opvt>>8) {
		case 0:
			// add segment to specified vtrack at currently calculated data offset
			aosp_add_seg(proj, opvt & 0xff, start, end, coff);
			// reset goto offset, save coff
			goff = 0;
			poff = coff;
			// calculate next block, round up to 64k
			aosp_next(proj, end-start, coff);
			break;

		case 2:
			// reset goto, save coff then skip segment
			goff = 0;
			poff = coff;
			aosp_next(proj, end-start, coff);
			break;

		case 4:
			// calculate goto offset
			goff = start * proj->splbyte;
			break;

		case 6:
			// apply goto offset, add segment data to specified vtrack
			aosp_add_seg(proj, opvt & 0xff, start, end, poff + goff);

		default:
			_DBG(_DBG_PRJ, "aosp: unknown segment table opvt=%04x\n", opvt);
			return -1;
		}
	}
	for (i=0; i<256; i++) {
		if (proj->vtracks[i].segs)
			aosp_sort_seg(i, &proj->vtracks[i]);
	}
	return 0;
}

void akaiosproject_clear(AkaiOsProject *proj) {
	int i;
	proj->offset = 0;
	if (proj->scenes)
		free(proj->scenes);
	proj->scenes = NULL;
	if (proj->vtracks) {
		for (i=0; i<256; i++) {
			while (proj->vtracks[i].segs) {
				AkaiOsSegment *t = proj->vtracks[i].segs->next;
				free(proj->vtracks[i].segs);
				proj->vtracks[i].segs = t;
			}
		} 	
		free(proj->vtracks);
	}
	proj->vtracks = NULL;
}

void akaiosproject_tracks(AkaiOsProject *proj, int (*cb)(char *trk, void *d), void *d) {
	if (proj && proj->vtracks) {
		int i, x=0;
		for (i=0; !x && i<256; i++) {
			if (proj->vtracks[i].channel && proj->vtracks[i].segs)
				x = cb(proj->vtracks[i].name, d);
		}
	}
}

void akaiosproject_mixes(AkaiOsProject *proj, int (*cb)(char *mix, void *d), void *d) {
}

void akaiosproject_memory(AkaiOsProject *proj, int (*cb)(char *mem, void *d), void *d) {
}

