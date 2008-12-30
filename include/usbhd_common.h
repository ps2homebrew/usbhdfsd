#ifndef _USBHD_COMMON_H
#define _USBHD_COMMON_H

#include <io_common.h>
#include <ioman.h>
//#include "fat.h"

#define USB_SUBCLASS_MASS_RBC		0x01
#define USB_SUBCLASS_MASS_ATAPI		0x02
#define USB_SUBCLASS_MASS_QIC		0x03
#define USB_SUBCLASS_MASS_UFI		0x04
#define USB_SUBCLASS_MASS_SFF_8070I 	0x05
#define USB_SUBCLASS_MASS_SCSI		0x06

#define USB_PROTOCOL_MASS_CBI		0x00
#define USB_PROTOCOL_MASS_CBI_NO_CCI	0x01
#define USB_PROTOCOL_MASS_BULK_ONLY	0x50

#define TAG_TEST_UNIT_READY     0
#define TAG_REQUEST_SENSE	3
#define TAG_INQUIRY		18
#define TAG_READ_CAPACITY       37
#define TAG_READ		40
#define TAG_START_STOP_UNIT	33
#define TAG_WRITE		42

#define DEVICE_DETECTED		1
#define DEVICE_CONFIGURED	2
#define DEVICE_DISCONNECTED 4

//#define MASS_CONNECT_CALLBACK    0x0012
//#define MASS_DISCONNECT_CALLBACK 0x0013

#define FAT_ERROR           -1

int fs_init   (iop_device_t *driver); 
int fs_open   (iop_file_t* , const char *name, int mode);
int fs_lseek  (iop_file_t* , unsigned long offset, int whence);
int fs_read   (iop_file_t* , void * buffer, int size );
int fs_write  (iop_file_t* , void * buffer, int size );
int fs_close  (iop_file_t* );
int fs_dummy  (void);

int fs_deinit (iop_device_t *);
int fs_format (iop_file_t *);
int fs_ioctl  (iop_file_t *, unsigned long, void *);
int fs_remove (iop_file_t *, const char *);
int fs_mkdir  (iop_file_t *, const char *);
int fs_rmdir  (iop_file_t *, const char *);
int fs_dopen  (iop_file_t *, const char *);
int fs_dclose (iop_file_t *);
int fs_dread  (iop_file_t *, fio_dirent_t *);
int fs_getstat(iop_file_t *, const char *, fio_stat_t *);

int fs_chstat (iop_file_t *, const char *, fio_stat_t *, unsigned int);

#endif // _USBHD_COMMON_H
