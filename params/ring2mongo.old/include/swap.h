
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: swap.h 4703 2012-02-10 20:15:30Z scott $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.4  2005/06/13 18:11:06  dietz
 *     add
 *
 *     Revision 1.3  2004/04/13 22:21:43  dietz
 *     added prototype for WaveMsg2MakeLocal()
 *
 *     Revision 1.2  2000/03/09 21:59:17  davidk
 *     added a prototype for SwapFloat(), it had not been inluded in the
 *     list of swap function prototypes.
 *
 *     Revision 1.1  2000/02/14 20:05:54  lucky
 *     Initial revision
 *
 *
 */

#ifndef SWAP_H
#define SWAP_H

#include <earthworm_simple_funcs.h>
#include <trace_buf.h>
/* include file for swap.c: handy routines for swapping earthwormy things */

void SwapShort( short * );                  /* swap.c       sys-independent  */
void SwapInt( int * );                      /* swap.c       sys-independent  */
void SwapLong ( long * );                   /* swap.c       sys-independent  */
void SwapDouble( double * );                /* swap.c       sys-independent  */
void SwapFloat( float * );                  /* swap.c       sys-independent  */

/* fixes wave message into local byte order, based on globals _SPARC and _INTEL */
int WaveMsgMakeLocal( TRACE_HEADER* );
int WaveMsg2MakeLocal( TRACE2_HEADER* );
int WaveMsg2XMakeLocal( TRACE2X_HEADER* );

#endif
