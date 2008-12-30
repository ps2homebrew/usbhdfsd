#ifndef _FAT_DRIVER_H
#define _FAT_DRIVER_H 1

#include "fat.h"

int fat_mountCheck(void);
void fat_forceUnmount(void); //dlanor: added for disconnection events (flush impossible)
void fat_setFatDirChain(fat_bpb* bpb, fat_dir* fatDir);
int fat_readFile(fat_bpb* bpb, fat_dir* fatDir, unsigned int filePos, unsigned char* buffer, int size);
int fat_getFirstDirentry(char * dirName, fat_dir* fatDir);
int fat_getNextDirentry(fat_dir* fatDir);

int getI32(unsigned char* buf);
int getI32_2(unsigned char* buf1, unsigned char* buf2);
int getI16(unsigned char* buf);
int strEqual(unsigned char *s1, unsigned char* s2);
unsigned int fat_getClusterRecord12(unsigned char* buf, int type);
unsigned int fat_cluster2sector(fat_bpb* bpb, unsigned int cluster);

int      fat_initDriver(void);
void     fat_closeDriver(void);
fat_bpb* fat_getBpb(void);
int      fat_getFileStartCluster(fat_bpb* bpb, const char* fname, unsigned int* startCluster, fat_dir* fatDir);
int      fat_getDirentrySectorData(fat_bpb* bpb, unsigned int* startCluster, unsigned int* startSector, int* dirSector);
unsigned int fat_cluster2sector(fat_bpb* bpb, unsigned int cluster);
int      fat_getDirentry(fat_direntry_sfn* dsfn, fat_direntry_lfn* dlfn, fat_direntry* dir );
int      fat_getClusterChain(fat_bpb* bpb, unsigned int cluster, unsigned int* buf, int bufSize, int start);
void     fat_invalidateLastChainResult();
void     fat_getClusterAtFilePos(fat_bpb* bpb, fat_dir* fatDir, unsigned int filePos, unsigned int* cluster, unsigned int* clusterPos);

#endif

