/*
 *
 *  mtcr.h - Mellanox Software tools (mst) driver definitions
 *
 */

#ifndef _MST_H
#define _MST_H


#ifdef __WIN__

#include <windows.h>

#ifdef MTCR_EXPORTS
#define MTCR_API __declspec(dllexport)
#else
#define MTCR_API __declspec(dllimport)
#endif

typedef unsigned __int8  u_int8_t;
typedef __int8           int8_t;
typedef unsigned __int16 u_int16_t;
typedef __int16          int16_t;
typedef unsigned __int32 u_int32_t;
typedef __int32          int32_t;
typedef unsigned __int64 u_int64_t;
typedef __int64          int64_t;

#if defined(_WIN64)
    typedef __int64 MT_long_ptr_t;
    typedef unsigned __int64 MT_ulong_ptr_t;
#else
    typedef _W64 long MT_long_ptr_t;
    typedef _W64 unsigned long MT_ulong_ptr_t;
#endif

#elif defined(__DJGPP__)

	typedef unsigned char    u_int8_t;
	typedef char             int8_t;
	typedef unsigned short   u_int16_t;
	typedef short            int16_t;
	typedef unsigned int     u_int32_t;
	typedef long             int32_t;
	typedef unsigned long long u_int64_t;
	typedef long long        int64_t;

#define bswap_32(x) ntohl(x)
#define MTCR_API
	
#else  /* UNIX */

#include <sys/types.h>
#define MTCR_API

#endif


#ifdef __cplusplus
extern "C" {
#endif

//#ifndef USE_IB_MGT
typedef struct mib_private_t {
    int  dummy;
} MIB_Private;
//#else
//#include "mtcr_ib_private.h"
//#endif

typedef enum MType_t {MST_PCI, MST_PCICONF, /*MST_CALBR,*/ MST_USB,
                        MST_IB, MST_IF, MST_PPC, MST_USB_DIMAX
#ifdef ENABLE_MST_DEV_I2C
                        ,MST_DEV_I2C
#endif
} MType;
typedef enum DType_t {MST_GAMLA, MST_TAVOR, MST_DIMM, MST_NOADDR} DType;
#define MST_ANAFA2 MST_TAVOR
#define MST_EEPROM MST_GAMLA
typedef enum Mdevs_t {
    MDEVS_GAMLA     = 0x01, /*  Each device that actually is a Gamla */
    MDEVS_I2CM      = 0x02, /*  Each device that can work as I2C master */
    MDEVS_MEM       = 0x04, /*  Each device that is a memory driver (vtop) */
    MDEVS_TAVOR_DDR = 0x08, /*  Each device that maps to Tavor DDR */
    MDEVS_TAVOR_UAR = 0x10, /*  Each device that maps to Tavor UAR */
    MDEVS_TAVOR_CR  = 0x20, /*  Each device that maps to Tavor CR */
    MDEVS_IF        = 0x40, /*  Standard device  interface */
    MDEVS_REM       = 0x80, /*  Remote devices */
    MDEVS_PPC       = 0x100, /*  PPC devices */
    MDEVS_DEV_I2C   = 0x200, /* Generic linux kernel i2c device */
    MDEVS_IB        = 0x400, /* Cr access over IB Mads */
    MDEVS_TAVOR     = (MDEVS_TAVOR_DDR|MDEVS_TAVOR_UAR|MDEVS_TAVOR_CR),
    MDEVS_ALL       = 0xffffffff
} Mdevs;

/*  All fields in follow structure are not supposed to be used */
/*  or modified by user programs. Except i2c_slave that may be */
/*  modified before each access to target I2C slave address */
typedef struct mfile_t {
    MType         tp;           /*  type of driver */
    DType         dtype;        /*  target device to access to */
    DType         itype;        /*  interface device to access via */
    int           is_i2cm;      /*  use device as I2C master */
    unsigned char i2c_slave;
#ifdef __WIN__
    MT_ulong_ptr_t fd;
#else
    int           fd;
#endif
    int           sock;         /*  in not -1 - remote interface */
    void         *ptr;
    unsigned int  map_size;
    MIB_Private   mib;          /*  Data for IB interface (if relevant) */
    unsigned int  i2c_RESERVED; /*  Reserved for internal usage (i2c internal) */
    enum Mdevs_t  flags;
    u_int32_t     hermon_wa_slot; /* apply hermon cr write workaround */
    int           hermon_wa_last_op_write;
    u_int32_t     hermon_wa_stat;
    u_int64_t     hermon_wa_max_retries;
    u_int64_t     hermon_wa_num_of_writes;
    u_int64_t     hermon_wa_num_of_retry_writes;
    int           server_ver_major;
    int           server_ver_minor;
} mfile;

#define SLV_ADDRS_NUM 128

#ifdef __WIN__
#define FromHandle(h)   ((MT_ulong_ptr_t)(h))
#define ToHandle(h)     ((HANDLE)(h))
#else
#define FromHandle(h)   ((int)(h))
#define ToHandle(h)     ((HANDLE)(h))
#endif

/*
 * Get list of MST (Mellanox Software Tools) devices.
 * Put all device names as null-terminated strings to buf.
 *
 * Return number of devices found or -1 if buf overflow
 */
MTCR_API int mdevices(char *buf, int len, int mask);

/*
 * Open Mellanox Software tools (mst) driver.
 * Return valid mfile ptr or 0 on failure
 */
MTCR_API mfile *mopend(const char *name, DType dtype);

/*
 * Open Mellanox Software tools (mst) driver. Device type==TAVOR
 * Return valid mfile ptr or 0 on failure
 */
MTCR_API mfile *mopen(const char *name);

/*
 * Close Mellanox driver
 * req. descriptor
 */
MTCR_API int mclose(mfile *mf);

/*
 * Accelerate device if possible.
 * When device is I2C master - overclock it
 */
MTCR_API void maccelerate(mfile *mf);

/*
 * Restore normal settings, if device was accelerated.
 */
MTCR_API void mrestore(mfile *mf);

/*
 * Read 4 bytes, return number of succ. read bytes or -1 on failure
 */
MTCR_API int mread4(mfile *mf, unsigned int offset, u_int32_t *value);

/*
 * Write 4 bytes, return number of succ. written bytes or -1 on failure
 */
MTCR_API int mwrite4(mfile *mf, unsigned int offset, u_int32_t value);

/*
 * Read a block of dwords, return number of succ. read bytes or -1 on failure
 * Works for any interface, but can be faster for interfaces where bursts
 * are supported (MTUSB, IB).
 * Data retrns in the same endianess of mread4/mwrite4
 */
MTCR_API int mread4_block (mfile *mf, unsigned int offset, u_int32_t* data, int byte_len);
MTCR_API int mwrite4_block(mfile *mf, unsigned int offset, u_int32_t* data, int byte_len);

/*
 * Read up to 64 bytes, return number of succ. read bytes or -1 on failure
 */
MTCR_API int mread64(mfile *mf, unsigned int offset, void *data, int length);

/*
 * Write up to 64 bytes, return number of succ. written bytes or -1 on failure
 */
MTCR_API int mwrite64(mfile *mf, unsigned int offset, void *data, int length);

/*
 * Read up to 64 bytes, return number of succ. read bytes or -1 on failure
 */
MTCR_API int mread_i2cblock(mfile *mf, unsigned char i2c_slave, u_int8_t addr_width,
                            unsigned int offset, void *data, int length);

/*
 * Write up to 64 bytes, return number of succ. written bytes or -1 on failure
 */
MTCR_API int mwrite_i2cblock(mfile *mf, unsigned char i2c_slave, u_int8_t addr_width,
                             unsigned int offset, void *data, int length);

/*
 * Set a new value for i2c_slave
 * Return previous value
 */
MTCR_API unsigned char mset_i2c_slave(mfile *mf, unsigned char new_i2c_slave);

MTCR_API int mset_i2c_addr_width(mfile *mf, u_int8_t  addr_width);
MTCR_API int mget_i2c_addr_width(mfile *mf, u_int8_t* addr_width);

MTCR_API int mget_mdevs_flags(mfile *mf, u_int32_t *devs_flags);
/*
 * Software reset the device.
 * Return 0 on success, <0 on failure.
 * Currently supported for IB device only.
 * Mellanox switch devices support this feature.
 * HCAs may not support this feature.
 */
MTCR_API int msw_reset(mfile *mf);

/*
 * get free phys. contigous pages
 * order should be in range [0..9]
 * the size of allocated memory will be (2^order * 4096)
 * return pointer to virtual address mapped to the allocated area
 * on failure returns 0 and errno is set
 */
MTCR_API void *mget_free_pages (mfile *mf, unsigned int order);

/*
 * free phys. contigous pages
 * order should be in range [0..9]
 * vma is freed
 * on success returns 0
 * on failure returns -1 and errno is set
 */
MTCR_API int mfree_pages (mfile *mf, void *addr, unsigned int order);



MTCR_API int mi2c_detect (mfile *mf, u_int8_t slv_arr[SLV_ADDRS_NUM]);

#define MTCR_MFT_2_7_0
MTCR_API int maccess_reg_mad(mfile *mf, u_int8_t *data);

/*
 * translate virtual address to physical address
 * return physical address on success, or 0 on error
 */
MTCR_API unsigned long mvtop (mfile *mf, void *va);

#ifdef __cplusplus
}
#endif


#endif
