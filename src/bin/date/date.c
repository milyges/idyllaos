/*
 * Idylla Operating System
 * Copyright (C) 2009-2012 Idylla Operating System Team
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
*/
#include <stdio.h>
#include <time.h>
#include <string.h>

int main(int argc, char * argv[])
{
	unsigned int month, day, hour, minute, year, century;
	char * format = "%c";
	struct tm tm;
	time_t systime;
	char buf[1024];
	
	if (argc > 1)
	{
		if (argv[1][0] == '+')
		{
			format = strdup(&argv[1][1]);
		}
		else if ((!strcmp(argv[1], "-h")) || (!strcmp(argv[1], "--help")))
		{
			printf("Usage:\n"
			       " %s [+FORMAT]          show date and time in format\n"
			       " %s [MMDDhhmm[[CC]YY]] set date and time\n", argv[0], argv[0]);
			return 1;
		}
		else
		{
			switch (strlen(argv[1]))
			{
				case 8:  sscanf(argv[1], "%02u%02u%02u%02u", &month, &day, &hour, &minute); break;
				case 10: sscanf(argv[1], "%02u%02u%02u%02u%02u", &month, &day, &hour, &minute, &year); break;
				case 12: sscanf(argv[1], "%02u%02u%02u%02u%02u%02u", &month, &day, &hour, &minute, &century, &year); break;
			}
		}
	}

	if (!format) 
		return 1;
	
	systime = time(NULL);
	localtime_r(&systime, &tm);
	strftime(buf, 1024, format, &tm);
	puts(buf);
	return 0;
}
