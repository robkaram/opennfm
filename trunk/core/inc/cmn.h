/*********************************************************
 * Module name: cmn.h
 *
 * Copyright 2010, 2011. All Rights Reserved, Crane Chu.
 *
 * This file is part of OpenNFM.
 *
 * OpenNFM is free software: you can redistribute it and/or 
 * modify it under the terms of the GNU General Public 
 * License as published by the Free Software Foundation, 
 * either version 3 of the License, or (at your option) any 
 * later version.
 * 
 * OpenNFM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied 
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
 * PURPOSE. See the GNU General Public License for more 
 * details.
 *
 * You should have received a copy of the GNU General Public 
 * License along with OpenNFM. If not, see 
 * <http://www.gnu.org/licenses/>.
 *
 * First written on 2010-01-01 by cranechu@gmail.com
 *
 * Module Description:
 *    The common type, value defination used by whole
 *    project. All define the overall compile option.
 *
 *********************************************************/


#ifndef _INC_CMN_H_
#define _INC_CMN_H_

#include <cfg.h>

/* for BOOL type */
#ifdef TRUE
#undef TRUE
#undef FALSE
#undef BOOL
#endif
#define TRUE   (1)
#define FALSE  (0)
typedef int    BOOL;


/* compile options */
#if (_MSC_VER && _MSC_VER >= 1200)
#define SIM_TEST           (TRUE)
#define DEBUG              (TRUE)
#define VERBOSE            (FALSE)
#else
#define SIM_TEST           (FALSE)
#define VERBOSE            (FALSE)
#ifdef NDEBUG
#define DEBUG              (FALSE)
#else
#define DEBUG              (TRUE)
#endif
#endif


/* base type */
typedef unsigned char   UINT8;
typedef unsigned short  UINT16;
typedef unsigned int    UINT32;


/* pointers to extern memory area */
typedef UINT8*   UINT8_PTR;
typedef UINT16*  UINT16_PTR;
typedef UINT32*  UINT32_PTR;


/* logical layer */
typedef UINT32          LSADDR;     /* logical sector address */
typedef UINT32          PGADDR;     /* logical block number */

/* extern block layer */
typedef UINT32          PHY_BLOCK;     /* block number in MTD */
typedef UINT32          LOG_BLOCK;     /* block number in UBI */
typedef UINT32          DIE_INDEX;
typedef UINT32          PMT_CLUSTER;
typedef PHY_BLOCK       ERASE_COUNT;/* erase count of the block */
typedef UINT32          AREA;       /* area number */
typedef UINT32          SECT_OFF;   /* sector offset in block */
typedef UINT32          PAGE_OFF;   /* page offset in block */
typedef UINT32          BLOCK_OFF;  /* block offset in block */

typedef UINT8           NAND_CHIP;


/* calculate other nand configuration */
#define SECTOR_SIZE                 (1<<SECTOR_SIZE_SHIFT)
#define SECTOR_PER_PAGE             (1<<SECTOR_PER_PAGE_SHIFT)
#define PAGE_SIZE                   (SECTOR_SIZE*SECTOR_PER_PAGE)
#define SECTOR_PER_MPP              (SECTOR_PER_PAGE*PLANE_PER_DIE)
#define SECTOR_PER_MPP_SHIFT        (SECTOR_PER_PAGE_SHIFT+PLANE_PER_DIE_SHIFT)
#define MPP_SIZE_SHIFT              (SECTOR_SIZE_SHIFT+SECTOR_PER_MPP_SHIFT)
#define MPP_SIZE                    (1<<MPP_SIZE_SHIFT)

#define PAGE_PER_PHY_BLOCK          (1<<PAGE_PER_BLOCK_SHIFT)

#define DIE_PER_CHIP                (1<<DIE_PER_CHIP_SHIFT)

#define PLANE_PER_DIE               (1<<PLANE_PER_DIE_SHIFT)


#define CHIP_COUNT                  (1<<CHIP_COUNT_SHIFT)

#define TOTAL_DIE_SHIFT             (DIE_PER_CHIP_SHIFT+CHIP_COUNT_SHIFT)
#define TOTAL_DIE_COUNT             (1<<TOTAL_DIE_SHIFT)

#define CFG_NAND_CHIP_COUNT         (1<<CHIP_COUNT_SHIFT)


#define CFG_PHY_BLOCK_COUNT_SHIFT   (BLOCK_PER_PLANE_SHIFT + \
                                     PLANE_PER_DIE_SHIFT + \
                                     DIE_PER_CHIP_SHIFT + \
                                     CHIP_COUNT_SHIFT)
#define CFG_LOG_BLOCK_COUNT_SHIFT   (BLOCK_PER_PLANE_SHIFT + \
                                     DIE_PER_CHIP_SHIFT + \
                                     CHIP_COUNT_SHIFT)
#define CFG_PHY_BLOCK_COUNT         (1<<CFG_PHY_BLOCK_COUNT_SHIFT)
#define CFG_LOG_BLOCK_COUNT         (1<<CFG_LOG_BLOCK_COUNT_SHIFT)

#define CFG_NAND_ROW_COUNT          (CFG_PHY_BLOCK_COUNT*PAGE_PER_PHY_BLOCK)

#define CFG_TOTAL_SECTOR_SHIFT      (SECTOR_PER_PAGE_SHIFT +   \
                                     PAGE_PER_BLOCK_SHIFT +    \
                                     BLOCK_PER_PLANE_SHIFT +   \
                                     PLANE_PER_DIE_SHIFT +     \
                                     TOTAL_DIE_SHIFT)
#define CFG_TOTAL_SECTOR_COUNT      (1<<CFG_TOTAL_SECTOR_SHIFT)

#define CFG_PHY_BLOCK_PER_AREA      (MPP_SIZE/2/sizeof(PHY_BLOCK))

#define AREA_COUNT                  ((CFG_LOG_BLOCK_COUNT +       \
                                      CFG_PHY_BLOCK_PER_AREA -    \
                                      1) /                        \
                                     CFG_PHY_BLOCK_PER_AREA)
#define AREA_TABLE_SIZE             (MPP_SIZE/2/2/sizeof(PHY_BLOCK))


/* data buffer im MPP size */
typedef UINT8           SECTOR[SECTOR_SIZE];
typedef SECTOR          PAGE_BUFFER[SECTOR_PER_MPP];


/* meta data in spare area */
#define SPARE_BYTES_IN_PAGE      (8)
#define SPARE_WORDS_IN_PAGE      (SPARE_BYTES_IN_PAGE/sizeof(UINT32))
typedef UINT32          SPARE[SPARE_WORDS_IN_PAGE];

/* buffer mgmt */
#define BUFFER_COUNT             (TOTAL_DIE_COUNT*2 + 2)

/* in a cycle of DMA burst, transfer 4 32bit word */
#define DMA_BURST_BYTES          (4*sizeof(unsigned int))

/* return status */
typedef enum {
   STATUS_SUCCESS = 0,

   /* SD status */
   STATUS_WRITE_PROTECTED,

   /* FTL */
   STATUS_BADBLOCK,
   STATUS_TOOMANY_BADBLOCK,
   STATUS_NO_DATA,
   STATUS_RECLAIM_NONE,
   STATUS_EMPTY_PAGE,
   STATUS_RAMBUFFER_FULL,
   STATUS_BDT_INIT_FAIL,
   STATUS_ROOT_INIT_FAIL,
   STATUS_HDI_INIT_FAIL,

   /* UBI */
   STATUS_UBI_FORMAT_ERROR,


   /* MTD */
   STATUS_DIE_BUSY,
   STATUS_ECC_ERROR,

   /* other */
   STATUS_PROGRAM_CHECKED,
   STATUS_WRITE_CACHE,
   STATUS_WRITE_REGROUP,
   STATUS_CACHE_WITH_REGROUP,
   STATUS_MERGE_WRITE_CACHE,
   STATUS_MERGE_WRITE_REGROUP,
   STATUS_FORMATTING,
   STATUS_NO_CONFIG,
   STATUS_NO_BUFFER,
   STATUS_ADDRESS_OVER,
   STATUS_WRITE_PROTECT,

   STATUS_SimulatedPowerLoss,
   STATUS_FAILURE = 0xff
} STATUS;


/* invalid value for return error */
#define INVALID_LSADDR  ((LSADDR)-1)
#define INVALID_OFFSET  ((SECT_OFF)-1)
#define INVALID_BLOCK   ((PHY_BLOCK)-1)
#define INVALID_AREA    ((AREA)-1)
#define INVALID_INDEX   ((UINT32)-1)
#define INVALID_PAGE    ((PAGE_OFF)-1)
#define INVALID_EC      ((ERASE_COUNT)-1)
#define INVALID_CLUSTER ((PMT_CLUSTER)-1)
#define INVALID_CHIP    ((NAND_CHIP)-1)

#define MAX_UINT8       ((UINT8)-1)
#define MAX_UINT32      ((UINT32)-1)


/* assert */
#ifdef ASSERT
#undef ASSERT
#endif
#if (DEBUG == TRUE)
#if (_MSC_VER && _MSC_VER >= 1200)
#include <assert.h>
#define ASSERT(exp)     assert((exp))
#else
#define ASSERT(exp)     while((exp) != TRUE)
#endif
#else
#define ASSERT(exp)
#endif


/* print log */
#ifdef DEBUG_LOG
#undef DEBUG_LOG
#endif
#if (DEBUG == TRUE && VERBOSE == TRUE && SIM_TEST == TRUE && _MSC_VER >= 1200)
/* pc sim in VC */
#include <stdio.h>
extern FILE* foutput;
#define DEBUG_LOG             \
         do    \
         {     \
            static UINT32  cycle_times = 0;        \
            printf(__FUNCTION__##"\n\r");          \
            fprintf(foutput, __FUNCTION__##": %d\n", cycle_times++);  \
         } while (FALSE)
#else
#define DEBUG_LOG             (FALSE)
#endif


/* for pointer in C51. The address 0 is a valid memory address in 51, so
 * we choose the highest address as null pointer address.
 * We only use ponter in xdata memory.
 */
#ifdef NULL
#undef NULL
#endif
#define NULL            ((void*)0)

/* min of two value */
#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif


/* uart for debug */
#if (SIM_TEST == FALSE)
#include "arm_comm.h"
#include "drv_uart.h"
#else
#define PRINTF(...)
#define GETCH(c)
#endif

#endif


