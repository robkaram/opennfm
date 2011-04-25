/*********************************************************
 * Module name: test_main.c
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
 *    main entrance of the simulated test system.
 *
 *********************************************************/


#include <core\inc\cmn.h>
#include <core\inc\ftl.h>
#include <core\inc\mtd.h>

#include <sys\sys.h>

#include <stdio.h>

#include "cutest-1.5\CuTest.h"
#include "suites.h"

#if (DEBUG == TRUE && VERBOSE == TRUE && SIM_TEST == TRUE && _MSC_VER >= 1200)
FILE* foutput;
#endif

/* can test on sim, and also on HW */

void RunAllTests()
{
   CuString *output = CuStringNew();
   CuSuite* suite = CuSuiteNew();

   /* add test suites here */
   CuSuiteAddSuite(suite, TestSuite_MTD());
   CuSuiteAddSuite(suite, TestSuite_UBI());
   CuSuiteAddSuite(suite, TestSuite_FTL());
   CuSuiteAddSuite(suite, TestSuite_BAT());

   CuSuiteRun(suite);
   CuSuiteSummary(suite, output);
   CuSuiteDetails(suite, output);
   printf("%s\n", output->buffer);
}


int main()
{
#if (DEBUG == TRUE && VERBOSE == TRUE && SIM_TEST == TRUE && _MSC_VER >= 1200)
   foutput = fopen("output.txt", "w");
#endif

   RunAllTests();

#if (DEBUG == TRUE && VERBOSE == TRUE && SIM_TEST == TRUE && _MSC_VER >= 1200)
   fclose(foutput);
#endif

   return 0;
}


