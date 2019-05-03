/*
	DATE2SEC.h
	Copyright (C) 2003 Bradford L. Barrett, Paul C. Pratt

	You can redistribute this file and/or modify it under the terms
	of version 2 of the GNU General Public License as published by
	the Free Software Foundation.  You should have received a copy
	of the license along with this file; see the file COPYING.

	This file is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	license for more details.
*/

/*
	DATE 2(to) SEConds

	convert year/month/day/hour/minute/second
	to number of seconds since the beginning
	of 1904, the format for storing dates
	on the Macintosh.

	The function jdate is from the program Webalizer
	by Bradford L. Barrett.
*/

#ifdef DATE2SEC_H
#error "header already included"
#else
#define DATE2SEC_H
#endif

/*
	The function jdate was found at the end of the file
	webalizer.c in the program webalizer at
	"www.mrunix.net/webalizer/".
	Here is copyright info from the top of that file:

	webalizer - a web server log analysis program

	Copyright (C) 1997-2000  Bradford L. Barrett (brad@mrunix.net)

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version, and provided that the above
	copyright and permission notice is included with all distributed
	copies of this or derived software.
*/

/* ************************************************************* */
/*                                                               */
/* JDATE  - Julian date calculator                               */
/*                                                               */
/* Calculates the number of days since Jan 1, 0000.              */
/*                                                               */
/* Originally written by Bradford L. Barrett (03/17/1988)        */
/* Returns an unsigned long value representing the number of     */
/* days since January 1, 0000.                                   */
/*                                                               */
/* Note: Due to the changes made by Pope Gregory XIII in the     */
/*       16th Centyry (Feb 24, 1582), dates before 1583 will     */
/*       not return a truely accurate number (will be at least   */
/*       10 days off).  Somehow, I don't think this will         */
/*       present much of a problem for most situations :)        */
/*                                                               */
/* Usage: days = jdate(day, month, year)                         */
/*                                                               */
/* The number returned is adjusted by 5 to facilitate day of     */
/* week calculations.  The mod of the returned value gives the   */
/* day of the week the date is.  (ie: dow = days % 7) where      */
/* dow will return 0=Sunday, 1=Monday, 2=Tuesday, etc...         */
/*                                                               */
/* ************************************************************* */

LOCALFUNC ui5b jdate(int day, int month, int year)
{
	ui5b days;                      /* value returned */
	int mtable[] = {
		0,    31,  59,  90, 120, 151,
		181, 212, 243, 273, 304, 334
	};

	/*
		First, calculate base number including leap
		and Centenial year stuff
	*/

	days = (((ui5b)year * 365) + day + mtable[month - 1]
		+ ((year + 4) / 4) - ((year / 100) - (year / 400)));

	/* now adjust for leap year before March 1st */

	if ((year % 4 == 0)
		&& (! ((year % 100 == 0) && (year % 400 != 0)))
		&& (month < 3))
	{
		--days;
	}

	/* done, return with calculated value */

	return (days + 5);
}

LOCALFUNC ui5b Date2MacSeconds(int second, int minute, int hour,
	int day, int month, int year)
{
	ui5b curjdate;
	ui5b basejdate;

	curjdate = jdate(day, month, year);
	basejdate = jdate(1, 1, 1904);
	return (((curjdate - basejdate) * 24 + hour) * 60
		+ minute) * 60 + second;
}
