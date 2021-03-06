#include "..\defs.h"
#include "..\drivers\ioman.h"
#ifndef _FS_H 
/* file system provide binary file abstraction and allocation for use with the upper layer */
/* the file system handle each file as plain binary format much like asgard 1.0 and fat based file system */
#define ASGARD_VERSION	4

#define FS_TYPE_DIR		0x8
#define FS_TYPE_USER	0x4
#define FS_TYPE_SYS		0x2
#define FS_TYPE_EXIST	0x80

#define FS_EOS			0xFFFF
#define FS_VOID			0x0000

//#define ALLOCATION_DATA_OFFSET 		512

#define actual_sector_size(x) (x << 5)
#define logical_sector_size(x) (x >> 5)
#define fs_fseek(handle, offset) { handle->data_ptr = offset; }

//#if ASGARD_VERSION==4

//#define ALLOCATION_DATA_OFFSET		512
//#define AW_32BIT		32			//32 bit system
//#define AW_16BIT		16			//16 bit system
//#define FS_ADDRESS_WIDTH	AW_16BIT

struct fs_table {
	uchar fs_type[3];		//file system tag
	uchar fd_ver;			//file system version (default (3) for asgard 3.0)
	uchar fs_mod;			//16 bit/32 bit (unused, reserved for future use)
	uchar sector_size;		//sector_size = actual_sector_size / 32, for 32byte use 1, for 512 use 16 and for 2048 use 64
	uint32 fs_size;			//file system size
};

struct fs_chain {
	/*#if FS_ADDRESS_WIDTH == AW_32BIT
    uint32 next;
	uint32 prev;
    uint32 size;
	#else */
	uint16 next;
	uint16 prev;
	uint16 size;
	//#endif
};

struct f_header {
	/*#if FS_ADDRESS_WIDTH == AW_32BIT
	uint32 parent;
	uint32 sibling;
	uint32 child;
	#else*/
	uint16 parent;
	uint16 sibling;
	uint16 child;
	//#endif
	uint16 fid;				//directory id
	uchar type;				//type (mf,df,ef)
};

struct df_header {
	/*#if FS_ADDRESS_WIDTH == AW_32BIT
	uint32 parent;
	uint32 sibling;
	uint32 child;
	#else*/
	uint16 parent;
	uint16 sibling;
	uint16 child;
	//#endif
	uint16 fid;				//directory id
	uchar type;				//type (mf,df,ef)
	uchar reserved1[5];		//reserved
	uchar length;			//always 9
	uchar file_char;		//file characteristic
	uchar num_of_df;		//number of df
	uchar num_of_ef;		//number of ef
	uchar num_of_chv;
};

struct ef_header {	
	/*#if FS_ADDRESS_WIDTH == AW_32BIT
	uint32 parent;
	uint32 sibling;
	uint32 child;
	#else */
	uint16 parent;
	uint16 sibling;
	uint16 child;			//used only for cyclic ef
	//#endif
	uint16 fid;				//file id
	uchar type;				//type (mf,df,ef)
	uint16 size;			//file size	
	uchar inc;				//increase allowed
	uchar acc_rw;			//access read/write
	uchar acc_inc;			//access increase
	uchar acc_ri;			//access inval/rehab
	uchar status;			//inval/rehab status 
	uchar length;			//always 2
	uchar structure;		//transparent, cyclic, linier
	uchar rec_size;			//record size
};

typedef struct f_header f_header;
typedef struct df_header df_header;
typedef struct ef_header ef_header;

typedef struct fs_table fs_table;
typedef struct fs_chain fs_chain; 


uint16 fs_init(void);
/*#if FS_ADDRESS_WIDTH == AW_32BIT
void fs_format(uint32 size);
uint32 fs_alloc(uint32 size);
void fs_dealloc(uint32 address);
#else */
void fs_format(uint32 size);
uint16 fs_alloc(uint16 size);
void fs_dealloc(uint16 address);
//#endif


//#endif

#define _FS_H
#endif





 