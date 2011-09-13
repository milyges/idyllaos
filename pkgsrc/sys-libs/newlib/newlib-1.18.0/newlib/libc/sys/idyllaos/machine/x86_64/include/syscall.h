/*
 * Idylla Operating System
 * Copyright (C) 2009-2010 Idylla Operating System Team
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
#ifndef __MACHINE_SYSCALL_H
#define __MACHINE_SYSCALL_H

#include <sys/syscall.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>



#define __SYSCALL0(num,ret) 
#define __SYSCALL1(num,ret,p1)
#define __SYSCALL2(num,ret,p1,p2)
#define __SYSCALL3(num,ret,p1,p2,p3) \
#define __SYSCALL5(num,ret,p1,p2,p3,p4,p5) \

#define __SYSCALL_EXIT(ret) \
        if ((int)(ret) < 0) \
        { \
         errno = -(int)(ret); \
         return -1; \
        } \
        return (ret)

#ifdef __cplusplus
}
#endif

#endif /* __MACHINE_SYSCALL_H */
