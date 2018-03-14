/**
 * @file
 * IEEE 802.15.4 interface
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
 
#ifndef SYLIXOS
# ifdef __GNUC__
#   define LW_INLINE inline
# else
#   define LW_INLINE
# endif /* __GNUC__ */
#else
#define __SYLIXOS_KERNEL
#endif /* !SYLIXOS */

#include "ieee802154_frame.h"

#include "string.h"

#define AUX_SEC_EN 0 /* not support now */

/*
 * Structure that contains the lengths of the various addressing and security fields
 * in the 802.15.4 header.  This structure is used in \ref ieee802154_frame_create()
 */
typedef struct {
  u8_t dest_pid_len;    /* Length (in bytes) of destination PAN ID field */
  u8_t dest_addr_len;   /* Length (in bytes) of destination address field */
  u8_t src_pid_len;     /* Length (in bytes) of source PAN ID field */
  u8_t src_addr_len;    /* Length (in bytes) of source address field */
  u8_t aux_sec_len;     /* Length (in bytes) of aux security header field */
} field_length_t;

/*
 * Get address length
 */
static LW_INLINE u8_t addr_len (u8_t mode)
{
  switch (mode) {
  case IEEE802154_SHORTADDRMODE:  /* 16-bit address */
    return 2;
  case IEEE802154_LONGADDRMODE:   /* 64-bit address */
    return 8;
  default:
    return 0;
  }
}

/*
 * Get field length
 */
static void field_len (ieee802154_frame_t *pframe, field_length_t *flen)
{
  /* init flen to zeros */
  memset(flen, 0, sizeof(field_length_t));
  
  /* Determine lengths of each field based on fcf and other args */
  if (pframe->fcf.dest_addr_mode & 3) {
    flen->dest_pid_len = 2;
  }
  
  if (pframe->fcf.src_addr_mode & 3) {
    flen->src_pid_len = 2;
  }

  /* Set PAN ID compression bit if src pan id matches dest pan id. */
  if (pframe->fcf.dest_addr_mode & 3 && pframe->fcf.src_addr_mode & 3 &&
    pframe->src_pid == pframe->dest_pid) {
    pframe->fcf.panid_compression = 1;

    /* compressed header, only do dest pid */
    flen->src_pid_len = 0;
  
  } else {
    pframe->fcf.panid_compression = 0;
  }

  /* determine address lengths */
  flen->dest_addr_len = addr_len((u8_t)(pframe->fcf.dest_addr_mode & 3));
  flen->src_addr_len = addr_len((u8_t)(pframe->fcf.src_addr_mode & 3));

  /* Aux security header */
  if (pframe->fcf.security_enabled & 1) {
#if AUX_SEC_EN > 0
    /* TODO Aux security header not yet implemented */
    switch (pframe->aux_hdr.security_control.key_id_mode) {
    case 0:
      flen->aux_sec_len = 5; /* minimum value */
      break;
    case 1:
      flen->aux_sec_len = 6;
      break;
    case 2:
      flen->aux_sec_len = 10;
      break;
    case 3:
      flen->aux_sec_len = 14;
      break;
    default:
      break;
    }
#endif /* AUX_SEC_EN > 0 */
  }
}

/**
 * Calculates the length of the frame header.  This function is
 * meant to be called by a higher level function, that interfaces to a MAC.
 *
 * @param pframe Pointer to ieee802154_frame_t struct, which specifies the
 *               frame to send.
 * @return The length of the frame header.
 */
u8_t ieee802154_frame_hdrlen (ieee802154_frame_t *pframe)
{
  field_length_t flen;
  
  field_len(pframe, &flen);
  
  return ((u8_t)(3 + flen.dest_pid_len + flen.dest_addr_len +
                 flen.src_pid_len + flen.src_addr_len + flen.aux_sec_len));
}

/**
 * Creates a frame for transmission over the air.  This function is
 * meant to be called by a higher level function (IP layer), that interfaces to a MAC.
 *
 * @param pframe Pointer to ieee802154_frame_t struct, which specifies the
 *               frame to send.
 * @param hdr_buf Pointer to the buffer to use for the frame header.
 * @param buf_len The length of the buffer to use for the frame header.
 * @return The length of the frame header or 0 if there was
 *         insufficient space in the buffer for the frame headers.
 */
u8_t ieee802154_frame_create_hdr (ieee802154_frame_t *pframe, u8_t *hdr_buf, u8_t buf_len)
{
  int c;
  field_length_t flen;
  u8_t *tx_frame_buffer;
  u8_t pos;

  field_len(pframe, &flen);
  
  if ((3 + flen.dest_pid_len + flen.dest_addr_len +
       flen.src_pid_len + flen.src_addr_len + flen.aux_sec_len) > buf_len) {
    /* Too little space for headers. */
    return 0;
  }
  
  /* OK, now we have field lengths.  Time to actually construct */
  /* the outgoing frame, and store it in tx_frame_buffer */
  tx_frame_buffer = hdr_buf;
  tx_frame_buffer[0] = (pframe->fcf.frame_type & 7) |
                       ((pframe->fcf.security_enabled & 1) << 3) |
                       ((pframe->fcf.frame_pending & 1) << 4) |
                       ((pframe->fcf.ack_required & 1) << 5) |
                       ((pframe->fcf.panid_compression & 1) << 6);
  tx_frame_buffer[1] = ((pframe->fcf.dest_addr_mode & 3) << 2) |
                       ((pframe->fcf.frame_version & 3) << 4) |
                       ((pframe->fcf.src_addr_mode & 3) << 6);
                       
  /* sequence number */
  tx_frame_buffer[2] = pframe->seq;
  pos = 3;
  
  /* Destination PAN ID */
  if (flen.dest_pid_len == 2) {
    tx_frame_buffer[pos++] = (pframe->dest_pid >> 8) & 0xff; 
    tx_frame_buffer[pos++] = (pframe->dest_pid & 0xff);
  }
  
  /* Destination address */
  for(c = flen.dest_addr_len; c > 0; c--) {
    tx_frame_buffer[pos++] = pframe->dest_addr[c - 1];
  }
  
  /* Source PAN ID */
  if(flen.src_pid_len == 2) {
    tx_frame_buffer[pos++] = (pframe->dest_pid >> 8) & 0xff;
    tx_frame_buffer[pos++] = (pframe->dest_pid & 0xff);
  }

  /* Source address */
  for(c = flen.src_addr_len; c > 0; c--) {
    tx_frame_buffer[pos++] = pframe->src_addr[c - 1];
  }

  /* Aux header */
  if(flen.aux_sec_len) {
#if AUX_SEC_EN > 0
    /* TODO Aux security header not yet implemented */
    pos += flen.aux_sec_len;
#endif /* AUX_SEC_EN > 0 */
  }
  
  return pos;
}

/**
 * Create a ACK packet.
 *
 * @param data The ack buf 
 * @param len The ack buf size
 * @param seqno ack seq no
 * @return ack length if successful
 */
u8_t ieee802154_frame_create_ack (u8_t *data, u8_t len, u8_t seqno)
{
  if (len >= 3) {
    data[0] = IEEE802154_ACKFRAME;
    data[1] = 0;
    data[2] = seqno;
    return 3;
  }
  return 0;
}

/**
 * Parses an input frame.  Scans the input frame to find each
 * section, and stores the information of each section in a
 * ieee802154_frame_t structure.
 *
 * @param data The input data from the radio chip.
 * @param len The size of the input data
 * @param pframe The ieee802154_frame_t struct to store the parsed frame information.
 * @return header length if successful
 */
u8_t ieee802154_frame_parse (u8_t *data, u16_t len, ieee802154_frame_t *pframe)
{
  u8_t *p;
  ieee802154_frame_fcf_t fcf;
  u8_t c;

  if(len < 3) {
    return 0;
  }
  
  p = data;
  
  /* decode the FCF */
  fcf.frame_type        = p[0] & 7;
  fcf.security_enabled  = (p[0] >> 3) & 1;
  fcf.frame_pending     = (p[0] >> 4) & 1;
  fcf.ack_required      = (p[0] >> 5) & 1;
  fcf.panid_compression = (p[0] >> 6) & 1;

  fcf.dest_addr_mode = (p[1] >> 2) & 3;
  fcf.frame_version  = (p[1] >> 4) & 3;
  fcf.src_addr_mode  = (p[1] >> 6) & 3;

  /* copy fcf and seqNum */
  memcpy(&pframe->fcf, &fcf, sizeof(ieee802154_frame_fcf_t));
  pframe->seq = p[2];
  p += 3;                             /* Skip first three bytes */
  
  /* Destination address, if any */
  if (fcf.dest_addr_mode) {
    /* Destination PAN */
    pframe->dest_pid = p[1] + (p[0] << 8);
    p += 2;
    
    if (fcf.dest_addr_mode == IEEE802154_SHORTADDRMODE) {
      memset(pframe->dest_addr, 0, IEEE802154_MAX_ADDR_LEN);
      pframe->dest_addr[0] = p[1];
      pframe->dest_addr[1] = p[0];
      p += 2;
      
    } else if(fcf.dest_addr_mode == IEEE802154_LONGADDRMODE) {
      for(c = 0; c < IEEE802154_MAX_ADDR_LEN; c++) {
        pframe->dest_addr[c] = p[IEEE802154_MAX_ADDR_LEN - c - 1];
      }
      p += IEEE802154_MAX_ADDR_LEN;
    }
  } else {
    memset(pframe->dest_addr, 0, IEEE802154_MAX_ADDR_LEN);
    pframe->dest_pid = 0;
  }
  
  /* Source address, if any */
  if (fcf.src_addr_mode) {
    /* Source PAN */
    if(!fcf.panid_compression) {
      pframe->src_pid = p[0] + (p[1] << 8);
      p += 2;
    } else {
      pframe->src_pid = pframe->dest_pid;
    }
    
    if (fcf.src_addr_mode == IEEE802154_SHORTADDRMODE) {
      memset(pframe->src_addr, 0, IEEE802154_MAX_ADDR_LEN);
      pframe->src_addr[0] = p[1];
      pframe->src_addr[1] = p[0];
      p += 2;
    
    } else if (fcf.src_addr_mode == IEEE802154_LONGADDRMODE) {
      for(c = 0; c < IEEE802154_MAX_ADDR_LEN; c++) {
        pframe->src_addr[c] = p[IEEE802154_MAX_ADDR_LEN - c - 1];
      }
      p += IEEE802154_MAX_ADDR_LEN;
    }
  } else {
    memset(pframe->src_addr, 0, IEEE802154_MAX_ADDR_LEN);
    pframe->src_pid = 0;
  }
  
  if (fcf.security_enabled) {
#if AUX_SEC_EN > 0
    /* TODO aux security header, not yet implemented */
#else
    return 0; /* not allow */
#endif
  }
  
  /* header length */
  c = p - data;
  
  /* payload length */
  pframe->payload_len = len - c;
  
  /* payload */
  pframe->payload = p;
  
  /* return header length if successful */
  return (u8_t)(c > len ? 0 : c);
}
/* end */