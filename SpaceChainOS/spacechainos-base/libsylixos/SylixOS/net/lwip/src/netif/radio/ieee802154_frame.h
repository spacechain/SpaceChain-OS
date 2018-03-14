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

#ifndef __IEEE802154_FRAME_H
#define __IEEE802154_FRAME_H

#include "lwip/opt.h"

#define IEEE802154_MAX_MTU              125        /* not include CRC16 */

#define IEEE802154_MIN_ADDR_LEN         2
#define IEEE802154_MAX_ADDR_LEN         8

/* These are some definitions of values used in the FCF.  See the 802.15.4 spec for details.
 * FCF element values definitions
 */
/* frame_type */
#define IEEE802154_BEACONFRAME          (0x00)
#define IEEE802154_DATAFRAME            (0x01)
#define IEEE802154_ACKFRAME             (0x02)
#define IEEE802154_CMDFRAME             (0x03)

#define IEEE802154_BEACONREQ            (0x07)

/* addr_mode */
#define IEEE802154_IEEERESERVED         (0x00)
#define IEEE802154_NOADDR               (0x00)      /* Only valid for ACK or Beacon frames. */
#define IEEE802154_SHORTADDRMODE        (0x02)
#define IEEE802154_LONGADDRMODE         (0x03)

#define IEEE802154_NOBEACONS            (0x0F)

#define IEEE802154_BROADCASTADDR        (0xFFFF)
#define IEEE802154_BROADCASTPANDID      (0xFFFF)

/* frame_version */
#define IEEE802154_IEEE802154_2003      (0x00)
#define IEEE802154_IEEE802154_2006      (0x01)

#define IEEE802154_SECURITY_LEVEL_NONE  (0)
#define IEEE802154_SECURITY_LEVEL_128   (3)

/*
 * The IEEE 802.15.4 frame has a number of constant/fixed fields that
 * can be counted to make frame construction and max payload
 * calculations easier.
 *
 * These include:
 *    1. FCF                  - 2 bytes       - Fixed
 *    2. Sequence number      - 1 byte        - Fixed
 *    3. Addressing fields    - 4 - 20 bytes  - Variable
 *    4. Aux security header  - 0 - 14 bytes  - Variable
 *    5. CRC                  - 2 bytes       - Fixed
*/
typedef struct {
  u8_t frame_type;        /* 3 bit. Frame type field, see 802.15.4 */
  u8_t security_enabled;  /* 1 bit. True if security is used in this frame */
  u8_t frame_pending;     /* 1 bit. True if sender has more data to send */
  u8_t ack_required;      /* 1 bit. Is an ack frame required? */
  u8_t panid_compression; /* 1 bit. Is this a compressed header? */
  u8_t reserved;          /* 3 bit. Unused bits */
  u8_t dest_addr_mode;    /* 2 bit. Destination address mode, see 802.15.4 */
  u8_t frame_version;     /* 2 bit. 802.15.4 frame version */
  u8_t src_addr_mode;     /* 2 bit. Source address mode, see 802.15.4 */
} ieee802154_frame_fcf_t;

/* 802.15.4 security control bitfield.  See section 7.6.2.2.1 in 802.15.4 specification */
typedef struct {
  u8_t  security_level; /* 3 bit. security level      */
  u8_t  key_id_mode;    /* 2 bit. Key identifier mode */
  u8_t  reserved;       /* 3 bit. Reserved bits       */
} ieee802154_frame_scf_t;

/* 802.15.4 Aux security header */
typedef struct {
  ieee802154_frame_fcf_t security_control;  /* Security control bitfield */
  u32_t frame_counter;   /* Frame counter, used for security */
  u8_t  key[9];          /* The key itself, or an index to the key */
} ieee802154_frame_aux_hdr_t;

/* Parameters used by the ieee802154_frame_create_hdr() function. These
 * parameters are used in the 802.15.4 frame header.  See the 802.15.4
 * specification for details.
 */
typedef struct {
  ieee802154_frame_fcf_t fcf; /* Frame control field  */
  u8_t  seq;            /* Sequence number */
  u16_t dest_pid;       /* Destination PAN ID */
  u8_t  dest_addr[IEEE802154_MAX_ADDR_LEN];   /* Destination address */
  u16_t src_pid;        /* Source PAN ID */
  u8_t  src_addr[IEEE802154_MAX_ADDR_LEN];    /* Source address */
  ieee802154_frame_aux_hdr_t aux_hdr; /* Aux security header */
  u8_t *payload;        /* Pointer to 802.15.4 frame payload */
  u8_t  payload_len;    /* Length of payload field */
} ieee802154_frame_t;

/* functions */
u8_t ieee802154_frame_hdrlen(ieee802154_frame_t *pframe);
u8_t ieee802154_frame_create_hdr(ieee802154_frame_t *pframe, u8_t *hdr_buf, u8_t buf_len);
u8_t ieee802154_frame_create_ack(u8_t *data, u8_t len, u8_t seqno);
u8_t ieee802154_frame_parse(u8_t *data, u16_t len, ieee802154_frame_t *pframe);

#endif /* __IEEE802154_FRAME_H */