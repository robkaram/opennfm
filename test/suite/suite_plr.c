/*********************************************************
 * Module name: suite_plr.c
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
 *    Test of Power Loss Recovery.
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


/* TODO: for now, only do PLR test on sim platform, and only do single PLR.
 * - PLR on init (doublePLR) not tested, though low risk.
 * - PLR on BBR not tested.
 */
 
#define TEST_DRIVER              (0)   /* or 'H' (non-zero DRIVER ID) for HW test */
#define PLR_SECTOR_PER_PHY_BLOCK  (0x20*4)

#if (TEST_DRIVER == 0)
/* sim test on pc ram */
#define LARGE_TEST_CYCLE         (0xffff)
#else
/* sim test on real HW usbkey or SD card */
#define LARGE_TEST_CYCLE         (0xfff)
#endif


static
STATUS plr_format();

static
STATUS plr_init();

static
STATUS plr_compare_ram_image(UINT8* image, UINT32 sector_count);

static
STATUS plr_compare_ram_image_plr(UINT8* image_origin, UINT8* image_updated, UINT32 sector_count);


void TC_PLR_HotBlockReadWrite(CuTest* tc)
{
   LSADDR   start_list[LARGE_TEST_CYCLE];
   UINT32   count_list[LARGE_TEST_CYCLE];
   UINT8    data_list[LARGE_TEST_CYCLE];
   UINT32   i;
   UINT32   j;
   UINT8    ram_image_origin[PLR_SECTOR_PER_PHY_BLOCK][SECTOR_SIZE];
   UINT8    ram_image_updated[PLR_SECTOR_PER_PHY_BLOCK][SECTOR_SIZE];
   LSADDR   start_sector;
   UINT32   sector_count;
   UINT8    write_data;
   STATUS   ret;
   BOOL     debug_mode = FALSE;
   UINT32   plr_target = 0;

   do
   {
      /* init ram image */
      memset(&(ram_image_origin[0][0]), 0xff, SECTOR_SIZE*PLR_SECTOR_PER_PHY_BLOCK);
      memset(&(ram_image_updated[0][0]), 0xff, SECTOR_SIZE*PLR_SECTOR_PER_PHY_BLOCK);

      /* init nand */
      ret = plr_format();
      if (ret == STATUS_SUCCESS)
      {
         ret = plr_init();
      }

      for (i=0; i<LARGE_TEST_CYCLE; i++)
      {
         if (ret == STATUS_SUCCESS)
         {
            /* seed the randome: no seed to freeze the test case */
            srand((int)(time(0)));
            srand(rand()+i);

            /* get sector */
            if (debug_mode == TRUE)
            {
               start_sector = start_list[i];
               sector_count = count_list[i];
               write_data = data_list[i];
            }
            else
            {
               start_sector = (LSADDR)(rand()%PLR_SECTOR_PER_PHY_BLOCK);
               sector_count = (UINT32)(rand()%(PLR_SECTOR_PER_PHY_BLOCK-start_sector));
               write_data   = (UINT8)(rand()%((UINT8)-1));
            }

            /* fill ram image with data */
            for (j=0; j<sector_count; j++)
            {
               memset(&(ram_image_updated[start_sector+j][0]),
                      write_data,
                      SECTOR_SIZE);
            }

            do
            {
               if (ret == STATUS_SimulatedPowerLoss)
               {
                  /* power loss, re-init and check */
                  ret = plr_init();
                  ASSERT(ret == STATUS_SUCCESS);
                  ret = plr_compare_ram_image_plr(&(ram_image_origin[0][0]),
                                                  &(ram_image_updated[0][0]),
                                                  PLR_SECTOR_PER_PHY_BLOCK);
               }

               if (ret == STATUS_SUCCESS)
               {
                  /* write and test PLR */
                  MTD_TestPLR(plr_target++);

                  ret = HWD_WriteSectors(TEST_DRIVER,
                                         start_sector,
                                         sector_count,
                                         &(ram_image_updated[start_sector][0]));

                  /* single PLR test: disable sim PL in init */
                  MTD_TestReset();
               }
            } while (ret == STATUS_SimulatedPowerLoss);

            plr_target = 0;
         }

         if (ret == STATUS_SUCCESS)
         {
            /* check */
            ret = plr_compare_ram_image(&(ram_image_updated[0][0]), PLR_SECTOR_PER_PHY_BLOCK);

            /* re-init and check */
            if (ret == STATUS_SUCCESS)
            {
               plr_init();

               ret = plr_compare_ram_image(&(ram_image_updated[0][0]), PLR_SECTOR_PER_PHY_BLOCK);
            }
         }

         if (debug_mode == FALSE)
         {
            start_list[i] = start_sector;
            count_list[i] = sector_count;
            data_list[i] = write_data;
         }

         if (ret != STATUS_SUCCESS)
         {
            printf("%6d:*FAIL* start address: %-4d, sector count: %-4d, data: %-4d \n\r", i, start_sector, sector_count, write_data);
#if (DEBUG == TRUE && VERBOSE == TRUE && SIM_TEST == TRUE && _MSC_VER >= 1200)
            fprintf(foutput, "%6d:*FAIL* start address: %-4d, sector count: %-4d, data: %-4d \n", i, start_sector, sector_count, write_data);
            fflush(foutput);
#endif
            debug_mode = FALSE;  /* set the TRUE if want to debug */
            ret = STATUS_SUCCESS;
            break;
         }
         else
         {
            /* update the ram image */
            memcpy(&(ram_image_origin[0][0]),
                   &(ram_image_updated[0][0]),
                   SECTOR_SIZE*PLR_SECTOR_PER_PHY_BLOCK);

            printf("PLR %6d: PASS. \n\r", i);
#if (DEBUG == TRUE && VERBOSE == TRUE && SIM_TEST == TRUE && _MSC_VER >= 1200)
            fprintf(foutput, "%6d: PASS. start address: %-4d, sector count: %-4d, data: %-4d \n", i, start_sector, sector_count, write_data);
#endif
         }
      }
   } while (debug_mode == TRUE);

   CuAssertTrue(tc, ret == STATUS_SUCCESS);
}


CuSuite* TestSuite_PLR()
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, TC_PLR_HotBlockReadWrite);

   return suite;
}


static
STATUS plr_format()
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
STATUS plr_init()
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
STATUS plr_compare_ram_image(UINT8* image, UINT32 sector_count)
{
   UINT8    data_buffer[SECTOR_SIZE];
   LSADDR   i;
   STATUS   ret = STATUS_SUCCESS;

   for (i=0; i<sector_count; i++)
   {
      (void)HWD_ReadSectors(TEST_DRIVER, i, 1, data_buffer);

      if (memcmp(data_buffer, image, SECTOR_SIZE) != 0)
      {
         ret = STATUS_FAILURE;
         break;
      }

      ret = STATUS_SUCCESS;
      image += SECTOR_SIZE;
   }

   return ret;
}


static
STATUS plr_compare_ram_image_plr(UINT8* image_origin, UINT8* image_updated, UINT32 sector_count)
{
   UINT8    data_buffer[SECTOR_SIZE];
   LSADDR   i;
   STATUS   ret = STATUS_SUCCESS;

   for (i=0; i<sector_count; i++)
   {
      (void)HWD_ReadSectors(TEST_DRIVER, i, 1, data_buffer);

      if (memcmp(data_buffer, image_updated, SECTOR_SIZE) != 0)
      {
         if (memcmp(data_buffer, image_origin, SECTOR_SIZE) != 0)
         {
            ret = STATUS_FAILURE;
            break;
         }
      }

      image_origin += SECTOR_SIZE;
      image_updated += SECTOR_SIZE;
   }

   return ret;
}


