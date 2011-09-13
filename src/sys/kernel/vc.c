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
#include <kernel/vc.h>
#include <lib/ctype.h>
#include <lib/string.h>

struct vc __vc_boot;
static struct vc * _cur_vc = &__vc_boot;

int vc_putch(char c, struct vc * vc)
{
	int err = 0, i;
	/* Sprawdzamystan sekwencji escape */
	switch(vc->esc)
	{
		case 0:
		{
			if (c == '\e') /* Znak escape */
			{
				vc->argc = 0;
				memset(vc->argv, 0, sizeof(unsigned) * VC_MAX_ARGS);
				vc->esc++;
			}
			else if (c == '\a') /* Dzwonek */
			{

			}
			else if (c == '\t') /* Tabulator */
			{
				vc->csr.col = (vc->csr.col + 8) & ~(8 - 1);
			}
			else if (c == '\b') /* Backspace */
			{
				if (vc->csr.col > 0) vc->csr.col--;
			}
			else if (c == '\r') /* Powrót karetki */
			{
				vc->csr.col = 0;
			}
			else if ((c == '\n') || (c == '\f')) /* Nowa linia */
			{
				vc->csr.col = 0;
				vc->csr.row++;
			}
			else if (c >= ' ')
			{
				err = vc->ops->putc(vc, c);
				if (err > 0)
					vc->csr.col += err;
			}
			break;
		}
		case 1: /* ESC */
		{
			if (c == '[')
				vc->esc++;
			else
				vc->esc = 0;
			break;
		}
		case 2: /* ESC[ */
		{
			if ((c == ';') && (vc->argc < VC_MAX_ARGS - 1))
			{
				vc->argc++;
				break;
			}
			else if (isdigit(c))
			{
				if (!vc->argc)
					vc->argc++;
				vc->argv[vc->argc - 1] = 10 * vc->argv[vc->argc - 1] + (c - '0');
				break;
			}
		}
		case 3: /* ESC[x */
		{
			/*** Cursor Control ***/
			switch(c)
			{
				case 'A': /* Przesuń kursor w górę */
				{
					if (!vc->argv[0])
						vc->argv[0]++;
					if (vc->csr.row - vc->argv[0] >= 0)
						vc->csr.row -= vc->argv[0];
					break;
				}
				case 'B': /* Przesuń kursor w dół */
				{
					if (!vc->argv[0])
						vc->argv[0]++;
					if (vc->csr.row + vc->argv[0] < vc->height)
						vc->csr.row += vc->argv[0];
					break;
				}
				case 'C': /* Przesuń kursor w prawo */
				{
					if (!vc->argv[0])
						vc->argv[0]++;
					if (vc->csr.col + vc->argv[0] < vc->width)
						vc->csr.col += vc->argv[0];
					break;
				}
				case 'D': /* Przesuń kursor w lewo */
				{
					if (!vc->argv[0])
						vc->argv[0]++;
					if (vc->csr.col - vc->argv[0] >= 0)
						vc->csr.col -= vc->argv[0];
					break;
				}
				case 'E': /* Przesuń kursor na początek lini i n lini w dół */
				{
					if (!vc->argv[0])
						vc->argv[0]++;
					vc->csr.col = 0;
					if (vc->csr.row + vc->argv[0] < vc->height)
						vc->csr.row += vc->argv[0];
					break;
				}
				case 'F': /* Przesuń kursor na początek lini i n lini w górę */
				{
					if (!vc->argv[0])
						vc->argv[0]++;
					vc->csr.col = 0;
					if (vc->csr.row - vc->argv[0] > 0)
						vc->csr.row -= vc->argv[0];
					break;
				}
				case 'G': /* Przesuń kursor do danej kolumny */
				{
					if (vc->argv[0])
						vc->argv[0]--;
					if (vc->argv[0] < vc->width)
						vc->csr.col = vc->argv[0];
					break;
				}
				case 'H': /* Ustaw pozycję kursora */
				case 'f':
				{
					if (vc->argv[0])
						vc->argv[0]--;
					if (vc->argv[1])
						vc->argv[1]--;
					vc->csr.row = vc->argv[0];
					vc->csr.col = vc->argv[1];
				}
				case 'h': /* TODO */
				{
					break;
				}
				case 's': /* Save current cursor position. */
				{
					if (vc->saves_count < VC_MAX_SAVES - 1)
						vc->saves[vc->saves_count++] = vc->csr;
					break;
				}
				case 'u': /* Restores cursor position after a Save Cursor. */
				{
					if (vc->saves_count > 0)
						vc->csr = vc->saves[--vc->saves_count];
					break;
				}
				case 'K': /* Wyczyść linię od pozycji kursora... */
				{
					vc->ops->clearline(vc, vc->argv[0]);
					break;
				}
				case 'J': /* Wyczyść ekran od aktualnej lini... */
				{
					vc->ops->clearscreen(vc, vc->argv[0]);
					break;
				}
				case 'l': /* TODO */
				{
					break;
				}
				case 'm': /* Ustaw atrybuty tekstu */
				{
					if (!vc->argc)
						vc->ops->setattr(vc, 0);
					else
					{
						for(i=0;i<vc->argc;i++)
							vc->ops->setattr(vc, vc->argv[i]);
					}
					break;
				}
			}
			vc->esc = 0;
		}
	}

	if (vc->csr.col >= vc->width)
	{
		vc->csr.col = 0;
		vc->csr.row++;
	}

	if (vc->csr.row >= vc->height)
	{
		vc->ops->scrollup(vc, 1);
		vc->csr.row--;
	}

	if (vc->is_active)
		vc->ops->movecsr(vc);

	return err;
}
