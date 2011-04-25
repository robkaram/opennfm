/*********************************************************
 * Module name: suite_bat.c
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
 *    ONFM Basical Acceptance Tests. 
 *
 *********************************************************/


#include <core\inc\cmn.h>
#include <core\inc\buf.h>
#include <core\inc\ftl.h>

#include <sys\sys.h>

#include <onfm.h>

#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "..\cutest-1.5\CuTest.h"
#include "..\suites.h"

#define  LARGE_TEST_CYCLE           (0xffffff)

static UINT32  USER_SPACE_SECTOR_COUNT;

/* whole virtual disk image for testing */
static SECTOR*          volumn_image;

static unsigned long    start_list[LARGE_TEST_CYCLE];
static unsigned long    count_list[LARGE_TEST_CYCLE];
static UINT8            data_list[LARGE_TEST_CYCLE];


#if defined(__ICCARM__)
#pragma data_alignment=16
#endif
SECTOR   check_data_buffer;

static
int bat_check_volumn_image()
{
   int      i;
   int      ret = 0;

   for (i=0; i<(int)USER_SPACE_SECTOR_COUNT; i++)
   {
      ret = ONFM_Read(i, 1, check_data_buffer);
      ASSERT(ret == 0);

      if (memcmp(check_data_buffer, volumn_image[i], SECTOR_SIZE) != 0)
      {
         ret = -1;
         break;
      }
      else
      {
         ret = 0;
      }
   }

   return ret;
}

extern UINT32  TEST_total_page_program;

/* check: cache reclaim for cache full or loggroup full. random stress test */
void TC_BAT_RandomReadWrite(CuTest* tc)
{
   unsigned long   start_sector = 0;
   unsigned long   sector_count = 0;
   unsigned long   rand_seed = 544;
   UINT8    write_data = 0;
   BOOL     debug_mode = FALSE;
   UINT32   i;
   UINT32   j;
   float    sector_written = 0;
   float    page_written = 0;
   float    wa = 1;
   int      ret;

   USER_SPACE_SECTOR_COUNT = ONFM_Capacity();
   volumn_image = (SECTOR*)malloc(SECTOR_SIZE*USER_SPACE_SECTOR_COUNT);

   do
   {
      /* init ram image */
      memset(&(volumn_image[0][0]), 0, SECTOR_SIZE*USER_SPACE_SECTOR_COUNT);

      /* init nand */
      ret = ONFM_Mount();
      if (ret != 0)
      {
         ret = ONFM_Format();
         ASSERT(ret == 0);

         BUF_Init();
         ret = FTL_Init();
      }

      ASSERT(ret == 0);

      for (i=0; i<LARGE_TEST_CYCLE; i++)
      {
         /* seed the randome: no seed to freeze the test case */
         //srand((int)(time(0)));
         //srand(rand()+i+rand_seed);

         /* get sector */
         if (debug_mode == TRUE)
         {
            start_sector = start_list[i];
            sector_count = count_list[i];
            write_data = data_list[i];
         }
         else
         {
            start_sector = (unsigned long)(rand()%USER_SPACE_SECTOR_COUNT);
            rand_seed = (unsigned long)(rand()%(USER_SPACE_SECTOR_COUNT-start_sector));
            write_data   = (UINT8)(rand()%((UINT8)-1));

            /* generate different length of write */
            if (write_data < 0x5)
            {
               sector_count = rand_seed;
            }
            else if (write_data < 0x10)
            {
               sector_count = rand_seed/10;
            }
            else if (write_data < 0x20)
            {
               sector_count = rand_seed/100;
            }
            else if (write_data < 0x30)
            {
               sector_count = rand_seed/1000;
            }
            else if (write_data < 0x80)
            {
               sector_count = 8;
            }
            else
            {
               sector_count = 1;
            }
         }

         /* fill ram image with data */
         for (j=0; j<sector_count; j++)
         {
            memset(&volumn_image[start_sector+j], write_data, SECTOR_SIZE);
         }

         /* the program in format should be eliminated to calc WA */
         TEST_total_page_program = 0;

         /* write */
         ret = ONFM_Write(start_sector, sector_count, &volumn_image[start_sector]);
         if (ret == 0)
         {
            /* calcuate WA */
            page_written += TEST_total_page_program;
            sector_written += sector_count;
            wa = page_written*SECTOR_PER_MPP/sector_written;
         }
         
         if(ret == 0 && i%0xfff == 0)
         {
            printf("Perform whole image checking ...");

            /* check */
            ret = bat_check_volumn_image();
            if (ret == 0)
            {
               printf(" SUCCESS! \n\r");
               printf("Re-init, PLR, and Perform whole image checking again ...");

               /* RE-init, and do PLR */
               BUF_Init();
               ret = FTL_Init();
            }

            if (ret == 0)
            {
               ret = bat_check_volumn_image();
            }

            if (ret == 0)
            {
               printf(" SUCCESS! \n\r");
            }
         }

         if (debug_mode == FALSE)
         {
            start_list[i] = start_sector;
            count_list[i] = sector_count;
            data_list[i] = write_data;
         }

         if (ret != 0)
         {
            printf("%6d:*FAIL* start address: %-4d, sector count: %-4d, data: %-4d \n\r", i, start_sector, sector_count, write_data);
#if (DEBUG == TRUE && VERBOSE == TRUE && SIM_TEST == TRUE && _MSC_VER >= 1200)
            fprintf(foutput, "%6d:*FAIL* start address: %-4d, sector count: %-4d, data: %-4d \n", i, start_sector, sector_count, write_data);
            fflush(foutput);
#endif
            debug_mode = TRUE;  /* set the TRUE if want to debug */
            //ret = STATUS_SUCCESS;
            break;
         }
         else
         {
            printf("%5d-PASS. start address: %-6d, sector count: %-6d, data: %-3d, WA = %.2f. \n\r",
                              i, start_sector, sector_count, write_data, wa);
#if (DEBUG == TRUE && VERBOSE == TRUE && SIM_TEST == TRUE && _MSC_VER >= 1200)
            fprintf(foutput, "%6d: PASS. start address: %-4d, sector count: %-4d, data: %-4d \n", i, start_sector, sector_count, write_data);
#endif
         }
      }
   } while (debug_mode == TRUE);

   CuAssertTrue(tc, ret == 0);
}


CuSuite* TestSuite_BAT()
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, TC_BAT_RandomReadWrite);

   return suite;
}


