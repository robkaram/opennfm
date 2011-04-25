/*********************************************************
 * Module name: suites.h
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
 *    Test Suites.
 *
 *********************************************************/


#ifndef _TEST_SUITES_H_
#define _TEST_SUITES_H_


/*********************************************************
 * Funcion Name: TestSuite_MTD
 *
 * Description:
 *    Test suite for MTD/nand
 *
 * Return Value:
 *    CuSuite
 *
 * Parameter List:
 *    N/A
 *
 * NOTES:
 *    N/A
 *
 *********************************************************/
CuSuite* TestSuite_MTD();


/*********************************************************
 * Funcion Name: TestSuite_MTD
 *
 * Description:
 *    Test suite for FTL's basical acceptance tests.
 *
 * Return Value:
 *    CuSuite
 *
 * Parameter List:
 *    N/A
 *
 * NOTES:
 *    N/A
 *
 *********************************************************/
CuSuite* TestSuite_BAT();


/*********************************************************
 * Funcion Name: TestSuite_PLR
 *
 * Description:
 *    Test suite for FTL's PLR tests.
 *
 * Return Value:
 *    CuSuite
 *
 * Parameter List:
 *    N/A
 *
 * NOTES:
 *    N/A
 *
 *********************************************************/
CuSuite* TestSuite_PLR();


/*********************************************************
 * Funcion Name: TestSuite_BBR
 *
 * Description:
 *    Test suite for FTL's BBR tests.
 *
 * Return Value:
 *    CuSuite
 *
 * Parameter List:
 *    N/A
 *
 * NOTES:
 *    N/A
 *
 *********************************************************/
CuSuite* TestSuite_BBR();


/*********************************************************
 * Funcion Name: TestSuite_UBI
 *
 * Description:
 *    Test suite for UBI
 *
 * Return Value:
 *    CuSuite
 *
 * Parameter List:
 *    N/A
 *
 * NOTES:
 *    N/A
 *
 *********************************************************/
CuSuite* TestSuite_UBI();


/*********************************************************
 * Funcion Name: TestSuite_FTL
 *
 * Description:
 *    Test suite for FTL.
 *
 * Return Value:
 *    CuSuite
 *
 * Parameter List:
 *    N/A
 *
 * NOTES:
 *    N/A
 *
 *********************************************************/
CuSuite* TestSuite_FTL();

#endif


