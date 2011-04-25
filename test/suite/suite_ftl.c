/*********************************************************
 * Module name: suite_ftl.c
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
 *    FTL page read write test.
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
#include <time.h>

#include "..\cutest-1.5\CuTest.h"
#include "..\suites.h"


void TC_FTL_BasicalValidation(CuTest* tc)
{
   STATUS   ret;
   UINT8    buffer[MPP_SIZE];

   MTD_Init();

   ret = FTL_Format();
   CuAssertTrue(tc, ret==STATUS_SUCCESS);

   ret = FTL_Init();
   CuAssertTrue(tc, ret==STATUS_SUCCESS);

   buffer[0] = 0x5a;
   ret = FTL_Write(0, buffer);
   CuAssertTrue(tc, ret==STATUS_SUCCESS);

   buffer[0] = 0x00;
   ret = FTL_Read(0, buffer);
   CuAssertTrue(tc, ret==STATUS_SUCCESS);
   CuAssertTrue(tc, buffer[0] == 0x5a);
}


CuSuite* TestSuite_FTL()
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, TC_FTL_BasicalValidation);

   return suite;
}



