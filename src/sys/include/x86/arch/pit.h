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
#ifndef __ARCH_PIT_H
#define __ARCH_PIT_H

#ifdef __CONFIG_TIMER_HZ
#define PIT_HZ       __CONFIG_TIMER_HZ
#else /* __CONFIG_TIMER_HZ */
#define PIT_HZ       250
#endif /* __CONFIG_TIMER_HZ */

#define PIT_IRQ      0

#define PIT_CHANNEL0 0x40
#define PIT_CHANNEL1 0x41
#define PIT_CHANNEL2 0x42
#define PIT_COMMAND  0x43

void pit_do_schedule(unsigned enabled);
void pit_delay(int ms);
void pit_init(void);

#endif /* __ARCH_PIT_H */
