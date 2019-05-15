/*
 *  drivers/s390/cio/chpid.h
 *
 *    Copyright IBM Corp. 2007
 *    Author(s): Peter Oberparleiter <peter.oberparleiter@de.ibm.com>
 */

#ifndef _ASM_S390_CHPID_H
#define _ASM_S390_CHPID_H _ASM_S390_CHPID_H

#include <linux/string.h>
#include <linux/types.h>

#define __MAX_CHPID 255

struct chp_id {
	u8 reserved1;
	u8 cssid;
	u8 reserved2;
	u8 id;
} __attribute__((packed));


#endif /* _ASM_S390_CHPID_H */
