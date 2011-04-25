/*********************************************************
 * Module name: suite_bbr.c
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
 *    Test of Bad Block Recovery.
 *
 *********************************************************/


#include <core\inc\cmn.h>
#include <core\inc\ftl.h>
#include <core\inc\mtd.h>

#include <sys\sys.h>

#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "..\cutest-1.5\CuTest.h"
#include "..\suites.h"


#define TEST_DRIVER                 (0)
#define BBR_SECTOR_PER_PHY_BLOCK    (0x80*4)
#define BBR_TEST_CYCLES             (5)


typedef struct
{
   UINT32   repeat_times;
   LSADDR   starting_sector;
   UINT32   sector_count;
} WRITE_OPERATION;


static
STATUS bbr_format();

static
STATUS bbr_init();

static
STATUS bbr_single_write(CuTest* tc, LSADDR start_addr, UINT32 sector_count);

static
void bbr_write_fail_test(CuTest*          tc,
                         WRITE_OPERATION  pre_writes[],
                         UINT32           nwrite,
                         WRITE_OPERATION  test_write);

static
void bbr_data_setup(UINT8_PTR data_buffer, UINT32 sector_count);

static
BOOL bbr_data_check(UINT8_PTR data_buffer, UINT32 sector_count);

static
void bbr_init_data(UINT8* data_buffer, UINT8 pad, UINT16 len);

static
BOOL bbr_check_data(UINT8* data_buffer, UINT8 pad);


void TC_BBR_RegroupWrite(CuTest* tc)
{
   WRITE_OPERATION pre_writes[] = {
                        {1, 0, 1},
                        {1, BBR_SECTOR_PER_PHY_BLOCK, 1},
                        {1, 2*BBR_SECTOR_PER_PHY_BLOCK, 1},
   };

   WRITE_OPERATION test_write = {1, 3*BBR_SECTOR_PER_PHY_BLOCK, 1};

   bbr_write_fail_test(tc,
                       pre_writes,
                       sizeof(pre_writes)/sizeof(WRITE_OPERATION),
                       test_write);
}


void TC_BBR_CacheWrite(CuTest* tc)
{
   WRITE_OPERATION pre_writes[] = {
                        {1, 0, 8},
   };

   WRITE_OPERATION test_write = {1, 8, 8};

   bbr_write_fail_test(tc,
                       pre_writes,
                       sizeof(pre_writes)/sizeof(WRITE_OPERATION),
                       test_write);
}


void TC_BBR_CacheWrite2(CuTest* tc)
{
   WRITE_OPERATION pre_writes[] = {
                        {1, 0, 8},
   };

   WRITE_OPERATION test_write = {1, 8, 16};

   bbr_write_fail_test(tc,
                       pre_writes,
                       sizeof(pre_writes)/sizeof(WRITE_OPERATION),
                       test_write);
}


void TC_BBR_CacheWriteWithRegroup(CuTest* tc)
{
   WRITE_OPERATION pre_writes[] = {
                        {1, 0, 1},
   };

   WRITE_OPERATION test_write = {1, 8, 8};

   bbr_write_fail_test(tc,
                       pre_writes,
                       sizeof(pre_writes)/sizeof(WRITE_OPERATION),
                       test_write);
}


void TC_BBR_CacheReclaim(CuTest* tc)
{
   WRITE_OPERATION pre_writes[] = {
                        {0x20-1, 0, 8},
   };

   WRITE_OPERATION test_write = {1, 8, 8};

   bbr_write_fail_test(tc,
                       pre_writes,
                       sizeof(pre_writes)/sizeof(WRITE_OPERATION),
                       test_write);
}


void TC_BBR_RegroupReclaim(CuTest* tc)
{
   WRITE_OPERATION pre_writes[] = {
                        {0x20-1, 0, 1},
   };

   WRITE_OPERATION test_write = {1, 0, 1};

   bbr_write_fail_test(tc,
                       pre_writes,
                       sizeof(pre_writes)/sizeof(WRITE_OPERATION),
                       test_write);
}


void TC_BBR_AreaAndIndexReclaim(CuTest* tc)
{
   WRITE_OPERATION pre_writes[] = {
                        {0x20*0x20-1, 0, 8},
   };

   WRITE_OPERATION test_write = {1, 0, 1};

   bbr_write_fail_test(tc,
                       pre_writes,
                       sizeof(pre_writes)/sizeof(WRITE_OPERATION),
                       test_write);
}


CuSuite* TestSuite_BBR()
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, TC_BBR_RegroupWrite);
   SUITE_ADD_TEST(suite, TC_BBR_CacheWrite);
   //SUITE_ADD_TEST(suite, TC_BBR_CacheWrite2);
   //SUITE_ADD_TEST(suite, TC_BBR_CacheWriteWithRegroup);
   SUITE_ADD_TEST(suite, TC_BBR_CacheReclaim);
   SUITE_ADD_TEST(suite, TC_BBR_RegroupReclaim);
   SUITE_ADD_TEST(suite, TC_BBR_AreaAndIndexReclaim);

   return suite;
}


static
void bbr_write_fail_test(CuTest*          tc,
                         WRITE_OPERATION  pre_writes[],
                         UINT32           nwrite,
                         WRITE_OPERATION  test_write)
{
   STATUS   ret;
   UINT32   i;
   UINT32   j;
   UINT32   bbr_target = 0;

   /* BBR test process:
    * - format
    * - write sectors to create test context
    * - enable BBR and set count
    * - write sectors to test BBR
    * - disable BBR
    * - check data
    * - increase BBR count, and next loop.
    */
   for (bbr_target=0; bbr_target<BBR_TEST_CYCLES; bbr_target++)
   {
      ret = bbr_format();
      if (ret == STATUS_SUCCESS)
      {
         ret = bbr_init();
      }

      for (i=0; i<nwrite; i++)
      {
         for (j=0; j<pre_writes[i].repeat_times; j++)
         {
            ret = bbr_single_write(tc,
                                   pre_writes[i].starting_sector,
                                   pre_writes[i].sector_count);
            CuAssertTrue(tc, ret==STATUS_SUCCESS);
         }
      }

      MTD_TestBBR(bbr_target);

      ret = bbr_single_write(tc,
                             test_write.starting_sector,
                             test_write.sector_count);
      CuAssertTrue(tc, ret==STATUS_SUCCESS);

      MTD_TestReset();
   }
}


static
STATUS bbr_format()
{
   STATUS   ret;

   FIFO_Init();
   NAND_Init();

   /* format the volume */
   ret = MPT_Init();
   if (ret == STATUS_SUCCESS)
   {
      ret = CHAIN_Init();
   }

   if (ret != STATUS_SUCCESS)
   {
      ret = MPT_Format(TEST_DRIVER, 2);
   }

   return ret;
}


static
STATUS bbr_init()
{
   STATUS ret;

   SD_InitDone();
   FIFO_Init();

   /* find the cfg table for the 1st time */
   ret = CHAIN_Init();
   if (ret == STATUS_SUCCESS)
   {
      /* find the updated cfg table */
      ret = ANCHOR_Init();
   }

   if (ret == STATUS_SUCCESS)
   {
      /* rebuild FTL running context */
      ret = FTL_Init();
   }

   return ret;
}


static
STATUS bbr_single_write(CuTest* tc, LSADDR start_addr, UINT32 sector_count)
{
   STATUS      ret;
   BOOL        bret;
   UINT8_PTR   data_buffer = (UINT8_PTR)malloc(SECTOR_SIZE*sector_count);

   /* setup data to write */
   bbr_data_setup(data_buffer, sector_count);

   /* write the data */
   ret = HWD_WriteSectors(TEST_DRIVER, start_addr, sector_count, data_buffer);
   if (ret == STATUS_SUCCESS)
   {
      /* read the data */
      ret = HWD_ReadSectors(TEST_DRIVER, start_addr, sector_count, data_buffer);
   }

   if (ret == STATUS_SUCCESS)
   {
      /* check data */
      bret = bbr_data_check(data_buffer, sector_count);
      if (bret != TRUE)
      {
         ret = STATUS_FAILURE;
      }
   }

   free(data_buffer);

   return ret;
}


static
void bbr_data_setup(UINT8_PTR data_buffer, UINT32 sector_count)
{
   UINT32   i;

   for (i=0; i<sector_count; i++)
   {
      bbr_init_data(data_buffer+i*SECTOR_SIZE, (UINT8)i, SECTOR_SIZE);
   }
}


static
BOOL bbr_data_check(UINT8_PTR data_buffer, UINT32 sector_count)
{
   UINT32   i;
   BOOL     ret = TRUE;

   for (i=0; i<sector_count; i++)
   {
      ret = bbr_check_data(data_buffer+i*SECTOR_SIZE, (UINT8)i);
      if (ret != TRUE)
      {
         break;
      }
   }

   return ret;
}


static
void bbr_init_data(UINT8* data_buffer, UINT8 pad, UINT16 len)
{
   memset(data_buffer, pad, len);
}


static
BOOL bbr_check_data(UINT8* data_buffer, UINT8 pad)
{
   BOOL     ret = TRUE;
   UINT8    target_data_buffer[SECTOR_SIZE];

   memset(target_data_buffer, pad, SECTOR_SIZE);
   if (memcmp(data_buffer, target_data_buffer, SECTOR_SIZE) != 0)
   {
      ret = FALSE;
   }

   return ret;
}


