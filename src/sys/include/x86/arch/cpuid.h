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
#ifndef __ARCH_CPUID_H
#define __ARCH_CPUID_H

/* CPUID commands */
#define CPUID_GETVENDOR   0x0
#define CPUID_GETFEATURES 0x1
#define CPUID_GETTLB      0x2
#define CPUID_GETSERIAL   0x3

/* CPUID flags */
#define CPUID_FLAG_FPU    0x1        /* Floating Point Unit. */
#define CPUID_FLAG_VME    0x2        /* Virtual Mode Extensions. */
#define CPUID_FLAG_DE     0x4        /* Debugging Extensions. */
#define CPUID_FLAG_PSE    0x8        /* Page Size Extensions. */
#define CPUID_FLAG_TSC    0x10       /* Time Stamp Counter. */
#define CPUID_FLAG_MSR    0x20       /* Model-specific registers. */
#define CPUID_FLAG_PAE    0x40       /* Physical Address Extensions. */
#define CPUID_FLAG_MCE    0x80       /* Machine Check Exceptions. */
#define CPUID_FLAG_CXCHG8 0x100      /* Compare and exchange 8-byte. */
#define CPUID_FLAG_APIC   0x200      /* On-chip APIC. */
#define CPUID_FLAG_SEP    0x800      /* Fast System Calls. */
#define CPUID_FLAG_MTRR   0x1000     /* Memory Type Range Registers.*/
#define CPUID_FLAG_PGE    0x2000     /* Page Global Enable.*/
#define CPUID_FLAG_MCA    0x4000     /* Machine Check Architecture. */
#define CPUID_FLAG_CMOV   0x8000     /* Conditional move-instruction. */
#define CPUID_FLAG_PAT    0x10000    /* Page Attribute Table. */
#define CPUID_FLAG_PSE36  0x20000    /* 36-bit Page Size Extensions. */
#define CPUID_FLAG_PSN    0x40000    /* Processor Serial Number. */
#define CPUID_FLAG_CLFL   0x80000    /* CLFLUSH - fixme? */
#define CPUID_FLAG_DTES   0x200000   /* Debug Trace and EMON Store MSRs. */
#define CPUID_FLAG_ACPI   0x400000   /* Thermal Cotrol MSR. */
#define CPUID_FLAG_MMX    0x800000   /* MMX instruction set. */
#define CPUID_FLAG_FXSR   0x1000000  /* Fast floating point save/restore. */
#define CPUID_FLAG_SSE    0x2000000  /* SSE (Streaming SIMD Extensions) */
#define CPUID_FLAG_SSE2   0x4000000  /* SSE2 (Streaming SIMD Extensions - #2) */
#define CPUID_FLAG_SS     0x8000000  /* Selfsnoop. */
#define CPUID_FLAG_HTT    0x10000000 /* Hyper-Threading Technology. */
#define CPUID_FLAG_TM1    0x20000000 /* Thermal Interrupts, Status MSRs. */
#define CPUID_FLAG_IA64   0x40000000 /* IA-64 (64-bit Intel CPU) */
#define CPUID_FLAG_PBE    0x80000000 /* Pending Break Event. */ 

#endif /* __ARCH_CPUID_H */
