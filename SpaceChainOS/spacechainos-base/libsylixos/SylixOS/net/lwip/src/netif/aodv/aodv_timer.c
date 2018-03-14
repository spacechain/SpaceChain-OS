/**
 * @file
 * The Ad hoc On Demand Distance Vector (AODV) routing protocol 
 * 
 * Verification using sylixos(tm) real-time operating system
 */

/*
 * Copyright (c) 2006-2014 SylixOS Group.
 * All rights reserved. 
 * 
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission. 
 * 4. This code has been or is applying for intellectual property protection 
 *    and can only be used with acoinfo software products.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 * OF SUCH DAMAGE.
 * 
 * Author: Han.hui <sylixos@gmail.com>
 *
 */
#include "lwip/opt.h"
#include "lwip/tcpip.h"

#include "aodv_list.h"
#include "aodv_timer.h"
#include "aodv_param.h"

static struct aodv_timer *timer_list; /* all active timer list */

/**
 * put a timer struct into a list.
 *
 * @param timer timer struct.
 * @param header timer list pointer.
 */
static void aodv_timer_inq (struct aodv_timer *timer, struct aodv_timer **header)
{
  __AODV_LIST_INSERT(timer, *header);
}

/**
 * remove a timer struct from a list.
 *
 * @param timer timer struct.
 * @param header timer list pointer.
 */
static void aodv_timer_outq (struct aodv_timer *timer, struct aodv_timer **header)
{
  __AODV_LIST_DELETE(timer, *header);
}

/**
 * lwip timer service function.
 *
 */
static void aodv_timer_tmr (void)
{
  struct aodv_timer *timer = timer_list;
  struct aodv_timer *timer_op;
  
  while (timer) {
    if (timer->msec > AODV_TIMER_PRECISION) {
      timer->msec -= AODV_TIMER_PRECISION;
      timer = timer->next; /* point to next one */
    } else {
      timer->msec = 0; /* need run handle */
      timer_op = timer;
      timer = timer->next; /* point to next one */
      
      /* delete from running queue */
      aodv_timer_remove(timer_op);
      
      /* run handle */
      if (timer_op->handle) {
        timer_op->handle(timer_op->arg);
      }
    }
  }
  
  sys_timeout(AODV_TIMER_PRECISION, (sys_timeout_handler)aodv_timer_tmr, NULL);
}

/**
 * initialize aodv service function.
 *
 */
void aodv_timer_init_svr (void)
{
#if LWIP_TCPIP_TIMEOUT
  /* May be not in tcpip thread, so we use tcpip_timeout() for safety */
  tcpip_timeout(AODV_TIMER_PRECISION, (sys_timeout_handler)aodv_timer_tmr, NULL);
#else
  sys_timeout(AODV_TIMER_PRECISION, (sys_timeout_handler)aodv_timer_tmr, NULL);
#endif
}

/**
 * initialize a aodv timer struct.
 *
 * @param timer timer struct.
 * @param handle timer handle function.
 * @param arg handle function argument.
 */
void aodv_timer_init (struct aodv_timer *timer, sys_timeout_handler  handle, void *arg)
{
  if (!timer) {
    return;
  }

  timer->next = NULL;
  timer->prev = NULL;
  
  timer->msec = 0;
  timer->handle = handle;
  timer->arg = arg;
  
  timer->isinq = 0;
}

/**
 * add a timer struct into active list.
 *
 * @param timer timer struct.
 */
void aodv_timer_add (struct aodv_timer *timer)
{
  if (!timer || !timer->handle) {
    return;
  }
  
  if (timer->isinq == 0) {
    aodv_timer_inq(timer, &timer_list);
    timer->isinq = 1;
  }
}

/**
 * remove a timer struct from active list.
 *
 * @param timer timer struct.
 * @return 0:not in the active list
 *         1:remove ok
 */
int aodv_timer_remove (struct aodv_timer *timer)
{
  if (!timer) {
    return 0;
  }
  
  if (timer->isinq) {
    aodv_timer_outq(timer, &timer_list);
    timer->isinq = 0;
    return 1;
  }
  
  return 0;
}

/**
 * run timer handle immediately.
 *
 * @param timer timer struct.
 * @return -1:not in the active list
 *          1:invoke ok
 */
int aodv_timer_timeout_now (struct aodv_timer *timer)
{
  if (aodv_timer_remove(timer)) {
    if (timer->handle) {
      timer->handle(timer->arg);
    }
    return 1;
  }
  
  return -1;
}

/**
 * set timer timeout
 *
 * @param timer timer struct.
 * @param msec timeout(ms)
 */
void aodv_timer_set_timeout (struct aodv_timer *timer, u32_t msec)
{
  if (!timer) {
    return;
  }
  
  if (timer->isinq) {
    aodv_timer_remove(timer);
  }
  
  timer->msec = msec;
  
  aodv_timer_add(timer);
}

/**
 * get timer left timeout.
 *
 * @param timer timer struct.
 * @return left timeout(ms)
 */
u32_t aodv_timer_left (struct aodv_timer *timer)
{
  if (!timer) {
    return 0;
  }
  
  return timer->msec;
}
/* end */
