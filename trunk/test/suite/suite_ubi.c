/*********************************************************
 * Module name: suite_ubi.c
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
 *    UBI module test. 
 *
 *********************************************************/


#include <core\inc\cmn.h>
#include <core\inc\ubi.h>
#include <core\inc\mtd.h>

#include <sys\sys.h>

#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "..\cutest-1.5\CuTest.h"
#include "..\suites.h"


void TC_UBI_BasicalValidation(CuTest* tc)
{
   STATUS   ret;
   UINT8    buffer[MPP_SIZE];
   SPARE    spare;

   MTD_Init();

   ret = UBI_Format();
   CuAssertTrue(tc, ret==STATUS_SUCCESS);

   ret = UBI_Init();
   CuAssertTrue(tc, ret==STATUS_SUCCESS);

   ret = UBI_Init();
   CuAssertTrue(tc, ret==STATUS_SUCCESS);

   MTD_Init();

   ret = UBI_Format();
   CuAssertTrue(tc, ret==STATUS_SUCCESS);

   ret = UBI_Init();
   CuAssertTrue(tc, ret==STATUS_SUCCESS);

   buffer[0] = 0x5a;
   spare[1] = 0xa5;
   ret = UBI_Write(0, 0, buffer, spare, FALSE);
   CuAssertTrue(tc, ret==STATUS_SUCCESS);

   buffer[0] = 0x00;
   spare[1] = 0x00;
   ret = UBI_Read(0, 0, buffer, spare);
   CuAssertTrue(tc, ret==STATUS_SUCCESS);
   CuAssertTrue(tc, buffer[0] == 0x5a);
   CuAssertTrue(tc, spare[1] == 0xa5);

   buffer[0] = 0xaa;
   spare[1] = 0x55;
   ret = UBI_Write(3, 12, buffer, spare, FALSE);
   CuAssertTrue(tc, ret==STATUS_SUCCESS);

   buffer[0] = 0x00;
   spare[1] = 0x00;
   ret = UBI_Read(3, 12, buffer, spare);
   CuAssertTrue(tc, ret==STATUS_SUCCESS);
   CuAssertTrue(tc, buffer[0] == 0xaa);
   CuAssertTrue(tc, spare[1] == 0x55);

   buffer[0] = 0x00;
   spare[1] = 0x00;
   ret = UBI_Read(3, 12, buffer, spare);
   CuAssertTrue(tc, ret==STATUS_SUCCESS);
   CuAssertTrue(tc, buffer[0] == 0xaa);
   CuAssertTrue(tc, spare[1] == 0x55);

   buffer[0] = 0x00;
   spare[1] = 0x00;
   ret = UBI_Read(3, 13, buffer, spare);
   CuAssertTrue(tc, ret==STATUS_FAILURE);
   CuAssertTrue(tc, buffer[0] == 0xff);
   CuAssertTrue(tc, spare[1] == 0xffffffff);

   ret = UBI_Erase(3, 43);
   CuAssertTrue(tc, ret==STATUS_SUCCESS);

   buffer[0] = 0x00;
   spare[1] = 0x00;
   ret = UBI_Read(3, 12, buffer, spare);
   CuAssertTrue(tc, ret==STATUS_FAILURE);
   CuAssertTrue(tc, buffer[0] == 0xff);
   CuAssertTrue(tc, spare[1] == 0xffffffff);

   buffer[0] = 0x00;
   spare[1] = 0x00;
   ret = UBI_Read(3, 13, buffer, spare);
   CuAssertTrue(tc, ret==STATUS_FAILURE);
   CuAssertTrue(tc, buffer[0] == 0xff);
   CuAssertTrue(tc, spare[1] == 0xffffffff);

   buffer[0] = 0xaa;
   spare[1] = 0x55;
   ret = UBI_Write(3, 12, buffer, spare, FALSE);
   CuAssertTrue(tc, ret==STATUS_SUCCESS);

   buffer[0] = 0x00;
   spare[1] = 0x00;
   ret = UBI_Read(3, 12, buffer, spare);
   CuAssertTrue(tc, ret==STATUS_SUCCESS);
   CuAssertTrue(tc, buffer[0] == 0xaa);
   CuAssertTrue(tc, spare[1] == 0x55);

   buffer[0] = 0x00;
   spare[1] = 0x00;
   ret = UBI_Read(3, 12, buffer, spare);
   CuAssertTrue(tc, ret==STATUS_SUCCESS);
   CuAssertTrue(tc, buffer[0] == 0xaa);
   CuAssertTrue(tc, spare[1] == 0x55);

   buffer[0] = 0x00;
   spare[1] = 0x00;
   ret = UBI_Read(3, 13, buffer, spare);
   CuAssertTrue(tc, ret==STATUS_FAILURE);
   CuAssertTrue(tc, buffer[0] == 0xff);
   CuAssertTrue(tc, spare[1] == 0xffffffff);
}


CuSuite* TestSuite_UBI()
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, TC_UBI_BasicalValidation);

   return suite;
}

