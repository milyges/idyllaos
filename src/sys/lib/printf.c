/*
 * Idylla Operating System
 * Copyright (C) 2009  Idylla Operating System Team
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
#include <lib/printf.h>
#include <lib/string.h>

#define PF_LJ    0x01  /* Wyrównanie do lewej */
#define PF_CA    0x02  /* Duże litery (A-F) */
#define PF_SG    0x04  /* Liczba ze znakiem */
#define PF_32    0x08  /* Liczba 32-bitowa */
#define PF_64    0x10  /* Liczba 64-bitowa */
#define PF_WS    0x20  /* Liczba była ze znakiem */
#define PF_PZ    0x40  /* Uzupełnij zerami */

#define PR_BUF   64

int doprintf(char * fmt, va_list args, int (*func)(unsigned c, void ** ptr), void * ptr)
{
	unsigned state, flags, radix, tmp, wd, cur_wd;
	int ret;
	char buf[PR_BUF];
	int64_t num;
	char * where;

	state = 0;
	radix = 10;
	flags = 0;
	ret = 0;
	wd = 0;

	for(;*fmt;fmt++)
	{
		switch(state)
		{
			case 0: /* STAN 0: Czekamy na % */
			{
				if (*fmt != '%')
					ret += func(*fmt, &ptr);
				else
					state++;
				break;
			}
			case 1: /* STAN 1: Czekamy na flagi %-0 */
			{
				if (*fmt == '%') /* %% */
				{
					ret += func(*fmt, &ptr);
					state = 0;
					flags = 0;
					wd = 0;
					break;
				}

				if (*fmt == '-')
				{
					flags |= PF_LJ;
					break;
				}

				if (*fmt == '0')
				{
					flags |= PF_PZ;
					fmt++;
				}
				state++;
			}
			case 2: /* STAN 2: Czekamy na szerokosc pola */
			{
				if((*fmt >= '0') && (*fmt <= '9'))
				{
					wd = 10 * wd + (*fmt - '0');
					break;
				}
				/* Tekst nie byl szerokoscia */
				state++;
			}
			case 3: /* STAN 3: Czekamy na modyfikator */
			{
				if (*fmt == 'l')
				{
					if (flags & PF_32) /* ll, lu, li, etc. */
					{
						flags |= PF_64;
						flags &= ~PF_32;
					}
					else
						flags |= PF_32;
					break;
				}

				state++;
			}
			case 4: /* STAN 4: Czekamy na znak konwersji */
			{
				where = buf + PR_BUF - 1;
				*where = '\0';
				switch (*fmt)
				{
					case 'p':
					{
						switch (sizeof(addr_t))
						{
							case 4: flags |= PF_32; if (!wd) wd = 8; break;
							case 8: flags |= PF_64; if (!wd) wd = 16; break;
						}
						flags |= PF_PZ;
					}
					case 'X': flags |= PF_CA;
					case 'x': radix = 16; goto DO_NUM;
					case 'd':
					case 'i': flags |= PF_SG;
					case 'u': radix = 10; goto DO_NUM;
					case 'o': radix = 8;
DO_NUM:
					if (flags & PF_64)
						num = va_arg(args, uint64_t);
					else if (flags & PF_32)
						num = va_arg(args, uint32_t);
					else
					{
						if (flags & PF_SG)
							num = va_arg(args, int);
						else
							num = va_arg(args, unsigned int);
					}
					/* Obsluga znaku */
					if (flags & PF_SG)
					{
						if (num < 0)
						{
							flags |= PF_WS;
							num = -num;
						}
					}
					/* Konwersja liczby na kody ASCII */
					do
					{
						tmp = (uint64_t)num % radix;
						where--;
						if (tmp < 10)
							*where = tmp + '0';
						else if (flags & PF_CA)
							*where = tmp - 10 + 'A';
						else
							*where = tmp - 10 + 'a';
						num = (uint64_t)num / radix;
					} while(num != 0);
					goto EMIT;
					
					case 'c':
					{
						flags &= ~PF_PZ;
						where--;
						*where = va_arg(args, int);
						cur_wd = 1;
						goto EMIT2;
					}
					case 's':
					{
						flags &= ~PF_PZ;
						where = va_arg(args, char *);
EMIT:
						cur_wd = strlen(where);
						if (flags & PF_WS)
							cur_wd++;

						/* Jezeli wypelniami zerami, wypisz znak */
						if((flags & (PF_WS | PF_PZ)) == (PF_WS | PF_PZ))
							ret += func('-', &ptr);

EMIT2:
						if((flags & PF_LJ) == 0)
						{
							while(wd > cur_wd)
							{
								ret += func((flags & PF_PZ) ? '0' : ' ', &ptr);
								wd--;
							}
						}

						/* Jezeli wypelniamy spacjamy, wypisz znak */
						if ((flags & (PF_WS | PF_PZ)) == PF_WS)
							ret += func('-', &ptr);

						/* Wypisz string/liczbe/etc. */
						while(*where != '\0')
							ret += func(*where++, &ptr);

						/* Dla wypelniania do lewej wypelnij spacjami */
						if (wd < cur_wd)
							wd = 0;
						else
							wd -= cur_wd;
						for(; wd; wd--)
							ret += func(' ', &ptr);
						break;
					}
					default:
					{
						break;
					}
				}
				default:
				{
					state = flags = wd = 0;
				}
			}
			break;
		}
	}

	return ret;
}

static int do_sprintf(unsigned c, void ** ptr)
{
	char * buf = *ptr;
	*buf++ = c;
	*ptr = buf;
	return sizeof(char);
}

char * sprintf(char * buf, char * fmt, ...)
{
	int len;
	va_list args;
	va_start(args, fmt);
	len = doprintf(fmt, args, &do_sprintf, buf);
	buf[len] = '\0';
	va_end(args);
	return buf;
}

char * snprintf(char * buf, size_t len, char * fmt, ...);