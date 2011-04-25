/*********************************************************
 * Module name: suite_mtd.c
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
 *    mtd and nand_sim test
 *
 *********************************************************/


#include <core\inc\cmn.h>
#include <core\inc\mtd.h>

#include <sys\sys.h>

#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>


#include "..\cutest-1.5\CuTest.h"
#include "..\suites.h"


void TC_MTD_Null(CuTest* tc)
{
	CuAssertIntEquals(tc, 1, 1);
}

void TC_MTD_ReadID(CuTest* tc)
{
   UINT8    id_data[5];
   STATUS   ret;

   ret = MTD_ReadID(1, id_data, 5);
   CuAssertTrue(tc, ret==STATUS_SUCCESS);
   CuAssertIntEquals(tc, 0xff, id_data[0]);
   CuAssertIntEquals(tc, 0xff, id_data[1]);
   CuAssertIntEquals(tc, 0x55, id_data[2]);
   CuAssertIntEquals(tc, 0x05, id_data[3]);
   CuAssertIntEquals(tc, 0x08, id_data[4]);
}

void TC_MTD_WriteOnePage(CuTest* tc)
{
   STATUS   ret;
   UINT8    buffer[MPP_SIZE];
   SPARE    spare;

   MTD_Init();

   /* prepare the buffer and data */
   memset(buffer, 0x5a, MPP_SIZE);
   spare[1] = 0xa5;

   /* write and check */
   ret = MTD_Program(1, 2, buffer, spare);
   CuAssertTrue(tc, ret==STATUS_SUCCESS);

   /* read back and check */
   ret = MTD_Read(1, 2, buffer, spare);
   CuAssertTrue(tc, ret==STATUS_SUCCESS);
   CuAssertTrue(tc, spare[1]==0xa5);
   CuAssertTrue(tc, buffer[1]==0x5a);
}


void TC_MTD_ReadEmptyPage(CuTest* tc)
{
   STATUS   ret;
   UINT8    buffer[MPP_SIZE];
   SPARE    spare;

   MTD_Init();

   /* prepare the buffer and data */
   memset(buffer, 0x5a, MPP_SIZE);
   spare[1] = 0xa5;

   /* write and check */
   ret = MTD_Program(0, 0, buffer, spare);
   CuAssertTrue(tc, ret==STATUS_SUCCESS);

   /* read back and check */
   ret = MTD_Read(3, 3, buffer, spare);
   CuAssertTrue(tc, ret==STATUS_FAILURE);
   CuAssertTrue(tc, spare[1]==0xffffffff);
   CuAssertTrue(tc, buffer[1]==0xff);
}


CuSuite* TestSuite_MTD()
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, TC_MTD_Null);
   SUITE_ADD_TEST(suite, TC_MTD_ReadID);
   SUITE_ADD_TEST(suite, TC_MTD_WriteOnePage);
   SUITE_ADD_TEST(suite, TC_MTD_ReadEmptyPage);

   return suite;
}

