//---------------------------------------------------------------------------
//File name:    scache.c
//---------------------------------------------------------------------------
/*
 * scache.c - USB Mass storage driver for PS2
 *
 * (C) 2004, Marek Olejnik (ole00@post.cz)
 * (C) 2004  Hermes (support for sector sizes from 512 to 4096 bytes)
 *
 * Sector cache
 *
 * See the file LICENSE included with this distribution for licensing terms.
 */
//---------------------------------------------------------------------------

#include <tamtypes.h>
#include <stdio.h>
#include <sysmem.h>
#define malloc(a)	AllocSysMemory(0,(a), NULL)
#define free(a)		FreeSysMemory((a))

#include "usbhd_common.h"
#include "mass_stor.h"

//---------------------------------------------------------------------------
// Modified Hermes
//always read 4096 bytes from sector (the rest bytes is stored in the cache)
#define READ_SECTOR_4096(d, a, b)	mass_stor_readSector4096((d), (a), (b))
#define WRITE_SECTOR_4096(d, a, b)	mass_stor_writeSector4096((d), (a), (b))

//#define DEBUG  //comment out this line when not debugging

#include "mass_debug.h"

//number of cache slots (1 slot = memory block of 4096 bytes)
#define CACHE_SIZE 32

//when the flushCounter reaches FLUSH_TRIGGER then flushSectors is called
#define FLUSH_TRIGGER 16

typedef struct _cache_record
{
	unsigned int sector;
	int tax;
	char writeDirty;
} cache_record;

typedef struct _cache_set
{
    int sectorSize;
    int indexLimit;
    unsigned char* sectorBuf; // = NULL;		//sector content - the cache buffer
    cache_record rec[CACHE_SIZE];	//cache info record

    //statistical infos
    unsigned int cacheAccess;
    unsigned int cacheHits;
    unsigned int writeFlag;
    unsigned int flushCounter;

    //unsigned int cacheDumpCounter = 0;
} cache_set;

cache_set* g_cache[NUM_DEVICES];

//---------------------------------------------------------------------------
int InitCache()
{
    int i;
	int ret = 0;
    for (i = 0; i < NUM_DEVICES; ++i)
        g_cache[i] = NULL;
    return ret;
}

//---------------------------------------------------------------------------
void initRecords(cache_set* cache)
{
	int i;

	for (i = 0; i < CACHE_SIZE; i++)
	{
		cache->rec[i].sector = 0xFFFFFFF0;
		cache->rec[i].tax = 0;
		cache->rec[i].writeDirty = 0;
	}

	cache->writeFlag = 0;
	cache->flushCounter = 0;
}

//---------------------------------------------------------------------------
/* search cache records for the sector number stored in cache
  returns cache record (slot) number
 */
int getSlot(cache_set* cache, unsigned int sector) {
	int i;

	for (i = 0; i < CACHE_SIZE; i++) {
		if (sector >= cache->rec[i].sector && sector < (cache->rec[i].sector + cache->indexLimit)) {
			return i;
		}
	}
	return -1;
}


//---------------------------------------------------------------------------
/* search cache records for the sector number stored in cache */
int getIndexRead(cache_set* cache, unsigned int sector) {
	int i;
	int index =-1;

	for (i = 0; i < CACHE_SIZE; i++) {
		if (sector >= cache->rec[i].sector && sector < (cache->rec[i].sector + cache->indexLimit)) {
			if (cache->rec[i].tax < 0) cache->rec[i].tax = 0;
			cache->rec[i].tax +=2;
			index = i;
		}
		cache->rec[i].tax--;     //apply tax penalty
	}
	if (index < 0)
		return index;
	else
		return ((index * cache->indexLimit) + (sector - cache->rec[index].sector));
}

//---------------------------------------------------------------------------
/* select the best record where to store new sector */
int getIndexWrite(int device, cache_set* cache, unsigned int sector) {
	int i, ret;
	int minTax = 0x0FFFFFFF;
	int index = 0;

	for (i = 0; i < CACHE_SIZE; i++) {
		if (cache->rec[i].tax < minTax) {
			index = i;
			minTax = cache->rec[i].tax;
		}
	}

	//this sector is dirty - we need to flush it first
	if (cache->rec[index].writeDirty) {
		XPRINTF("scache: getIndexWrite: sector is dirty : %d   index=%d \n", cache->rec[index].sector, index);
		ret = WRITE_SECTOR_4096(mass_stor_getDevice(device), cache->rec[index].sector, cache->sectorBuf + (index * 4096));
		cache->rec[index].writeDirty = 0;
		//TODO - error handling
		if (ret < 0) {
			printf("scache: ERROR writing sector to disk! sector=%d\n", sector);
		}

	}
	cache->rec[index].tax +=2;
	cache->rec[index].sector = sector;

	return index * cache->indexLimit;
}



//---------------------------------------------------------------------------
/*
	flush dirty sectors
 */
int scache_flushSectors(int device) {
    cache_set* cache = g_cache[device];
	int i,ret;
	int counter = 0;

	XPRINTF("cache: flushSectors device = %i \n", device);

    if (cache == NULL) {
        return 0;
    }

	cache->flushCounter = 0;

	XPRINTF("scache: flushSectors writeFlag=%d\n", cache->writeFlag);
	//no write operation occured since last flush
	if (cache->writeFlag==0) {
		return 0;
	}

	for (i = 0; i < CACHE_SIZE; i++) {
		if (cache->rec[i].writeDirty) {
			XPRINTF("scache: flushSectors dirty index=%d sector=%d \n", i, cache->rec[i].sector);
			ret = WRITE_SECTOR_4096(mass_stor_getDevice(device), cache->rec[i].sector, cache->sectorBuf + (i * 4096));
			cache->rec[i].writeDirty = 0;
			//TODO - error handling
			if (ret < 0) {
				printf("scache: ERROR writing sector to disk! sector=%d\n", cache->rec[i].sector);
				return ret;
			}
			counter ++;
		}
	}
	cache->writeFlag = 0;
	return counter;
}

//---------------------------------------------------------------------------
int scache_readSector(int device, unsigned int sector, void** buf) {
    cache_set* cache = g_cache[device];
	int index; //index is given in single sectors not octal sectors
	int ret;
	unsigned int alignedSector;

	XPRINTF("cache: readSector device = %i sector = %i \n", device, sector);
    if (cache == NULL) {
        XPRINTF("cache: device cache not created = %i \n", device);
        return -1;
    }
    
	cache->cacheAccess ++;
	index = getIndexRead(cache, sector);
	XPRINTF("cache: indexRead=%i \n", index);
	if (index >= 0) { //sector found in cache
		cache->cacheHits ++;
		*buf = cache->sectorBuf + (index * cache->sectorSize);
		XPRINTF("cache: hit and done reading sector \n");

		return cache->sectorSize;
	}

	//compute alignedSector - to prevent storage of duplicit sectors in slots
	alignedSector = (sector/cache->indexLimit)*cache->indexLimit;
	index = getIndexWrite(device, cache, alignedSector);
	XPRINTF("cache: indexWrite=%i slot=%d  alignedSector=%i\n", index, index / cache->indexLimit, alignedSector);
	ret = READ_SECTOR_4096(mass_stor_getDevice(device), alignedSector, cache->sectorBuf + (index * cache->sectorSize));

	if (ret < 0) {
		return ret;
	}
	*buf = cache->sectorBuf + (index * cache->sectorSize) + ((sector%cache->indexLimit) * cache->sectorSize);
	XPRINTF("cache: done reading physical sector \n");

	//write precaution
	cache->flushCounter++;
	if (cache->flushCounter == FLUSH_TRIGGER) {
		scache_flushSectors(device);
	}

	return cache->sectorSize;
}


//---------------------------------------------------------------------------
int scache_allocSector(int device, unsigned int sector, void** buf) {
    cache_set* cache = g_cache[device];
	int index; //index is given in single sectors not octal sectors
	//int ret;
	unsigned int alignedSector;

	XPRINTF("cache: allocSector device = %i sector = %i \n", device, sector);
	if (cache == NULL) {
		XPRINTF("cache: device cache not created = %i \n", device);
		return -1;
	}
    
	index = getIndexRead(cache, sector);
	XPRINTF("cache: indexRead=%i \n", index);
	if (index >= 0) { //sector found in cache
		*buf = cache->sectorBuf + (index * cache->sectorSize);
		XPRINTF("cache: hit and done allocating sector \n");
		return cache->sectorSize;
	}

	//compute alignedSector - to prevent storage of duplicit sectors in slots
	alignedSector = (sector/cache->indexLimit)*cache->indexLimit;
	index = getIndexWrite(device, cache, alignedSector);
	XPRINTF("cache: indexWrite=%i \n", index);
	*buf = cache->sectorBuf + (index * cache->sectorSize) + ((sector%cache->indexLimit) * cache->sectorSize);
	XPRINTF("cache: done allocating sector\n");
	return cache->sectorSize;
}


//---------------------------------------------------------------------------
int scache_writeSector(int device, unsigned int sector) {
    cache_set* cache = g_cache[device];
	int index; //index is given in single sectors not octal sectors
	//int ret;

	XPRINTF("cache: writeSector device = %i sector = %i \n", device, sector);
	if (cache == NULL) {
		XPRINTF("cache: device cache not created = %i \n", device);
		return -1;
	}

	index = getSlot(cache, sector);
	if (index <  0) { //sector not found in cache
		XPRINTF("cache: writeSector: ERROR! the sector is not allocated! \n");
		return -1;
	}
	XPRINTF("cache: slotFound=%i \n", index);

	//prefere written sectors to stay in cache longer than read sectors
	cache->rec[index].tax += 2;

	//set dirty status
	cache->rec[index].writeDirty++;
	cache->writeFlag++;

	XPRINTF("cache: done soft writing sector \n");


	//write precaution
	cache->flushCounter++;
	if (cache->flushCounter == FLUSH_TRIGGER) {
		scache_flushSectors(device);
	}

	return cache->sectorSize;
}

//---------------------------------------------------------------------------
int scache_init(int device, int sectSize)
{
	XPRINTF("cache: init device = %i sectSize = %i \n", device, sectSize);

    if (g_cache[device] == NULL)
    {
        g_cache[device] = malloc(sizeof(cache_set));
        g_cache[device]->sectorBuf = NULL;
    }
    cache_set* cache = g_cache[device];
        
	//added by Hermes
	cache->sectorSize = sectSize;
	cache->indexLimit = 4096/cache->sectorSize; //number of sectors per 1 cache slot

	if (cache->sectorBuf == NULL) {
		XPRINTF("scache init! \n");
		XPRINTF("sectorSize: 0x%x\n", cache->sectorSize);
		cache->sectorBuf = (unsigned char*) malloc(4096 * CACHE_SIZE ); //allocate 4096 bytes per 1 cache record
		if (cache->sectorBuf == NULL) {
			XPRINTF("Sector cache: can't alloate memory of size:%d \n", 4096 * CACHE_SIZE);
			return -1;
		}
		XPRINTF("Sector cache: alocated memory at:%p of size:%d \n", cache->sectorBuf, 4096 * CACHE_SIZE);
	} else {
		XPRINTF("scache flush! \n");
	}
	cache->cacheAccess = 0;
	cache->cacheHits = 0;
	initRecords(cache);
	return(1);
}

//---------------------------------------------------------------------------
void scache_getStat(int device, unsigned int* access, unsigned int* hits) {
    cache_set* cache = g_cache[device];
	if (cache == NULL) {
		XPRINTF("cache: device cache not created = %i \n", device);
		return;
	}
	*access = cache->cacheAccess;
	*hits = cache->cacheHits;
}

//---------------------------------------------------------------------------
void scache_kill(int device) //dlanor: added for disconnection events (flush impossible)
{
	XPRINTF("cache: kill device = %i \n", device);
    
	cache_set* cache = g_cache[device];
	if (cache == NULL) {
		XPRINTF("cache: device cache not created = %i \n", device);
		return;
	}
	if(cache->sectorBuf != NULL)
	{
		free(cache->sectorBuf);
		cache->sectorBuf = NULL;
	}
}
//---------------------------------------------------------------------------
void scache_close(int device)
{
	XPRINTF("cache: close device = %i \n", device);
	scache_flushSectors(device);
	scache_kill(device);
}
//---------------------------------------------------------------------------
//End of file:  scache.c
//---------------------------------------------------------------------------
