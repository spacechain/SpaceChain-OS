#ifndef __ALSA_SEQ_EVENT_H
#define __ALSA_SEQ_EVENT_H

typedef unsigned char snd_seq_event_type_t;

enum snd_seq_event_type {
	SND_SEQ_EVENT_SYSTEM = 0,
	SND_SEQ_EVENT_RESULT,

	SND_SEQ_EVENT_NOTE = 5,
	SND_SEQ_EVENT_NOTEON,
	SND_SEQ_EVENT_NOTEOFF,
	SND_SEQ_EVENT_KEYPRESS,
	
	SND_SEQ_EVENT_CONTROLLER = 10,
	SND_SEQ_EVENT_PGMCHANGE,
	SND_SEQ_EVENT_CHANPRESS,
	SND_SEQ_EVENT_PITCHBEND,
	SND_SEQ_EVENT_CONTROL14,
	SND_SEQ_EVENT_NONREGPARAM,
	SND_SEQ_EVENT_REGPARAM,

	SND_SEQ_EVENT_SONGPOS = 20,
	SND_SEQ_EVENT_SONGSEL,
	SND_SEQ_EVENT_QFRAME,
	SND_SEQ_EVENT_TIMESIGN,
	SND_SEQ_EVENT_KEYSIGN,
	        
	SND_SEQ_EVENT_START = 30,
	SND_SEQ_EVENT_CONTINUE,
	SND_SEQ_EVENT_STOP,
	SND_SEQ_EVENT_SETPOS_TICK,
	SND_SEQ_EVENT_SETPOS_TIME,
	SND_SEQ_EVENT_TEMPO,
	SND_SEQ_EVENT_CLOCK,
	SND_SEQ_EVENT_TICK,
	SND_SEQ_EVENT_QUEUE_SKEW,
	SND_SEQ_EVENT_SYNC_POS,

	SND_SEQ_EVENT_TUNE_REQUEST = 40,
	SND_SEQ_EVENT_RESET,
	SND_SEQ_EVENT_SENSING,

	SND_SEQ_EVENT_ECHO = 50,
	SND_SEQ_EVENT_OSS,

	SND_SEQ_EVENT_CLIENT_START = 60,
	SND_SEQ_EVENT_CLIENT_EXIT,
	SND_SEQ_EVENT_CLIENT_CHANGE,
	SND_SEQ_EVENT_PORT_START,
	SND_SEQ_EVENT_PORT_EXIT,
	SND_SEQ_EVENT_PORT_CHANGE,

	SND_SEQ_EVENT_PORT_SUBSCRIBED,
	SND_SEQ_EVENT_PORT_UNSUBSCRIBED,

	SND_SEQ_EVENT_USR0 = 90,
	SND_SEQ_EVENT_USR1,
	SND_SEQ_EVENT_USR2,
	SND_SEQ_EVENT_USR3,
	SND_SEQ_EVENT_USR4,
	SND_SEQ_EVENT_USR5,
	SND_SEQ_EVENT_USR6,
	SND_SEQ_EVENT_USR7,
	SND_SEQ_EVENT_USR8,
	SND_SEQ_EVENT_USR9,

	SND_SEQ_EVENT_SYSEX = 130,
	SND_SEQ_EVENT_BOUNCE,

	SND_SEQ_EVENT_USR_VAR0 = 135,
	SND_SEQ_EVENT_USR_VAR1,
	SND_SEQ_EVENT_USR_VAR2,
	SND_SEQ_EVENT_USR_VAR3,
	SND_SEQ_EVENT_USR_VAR4,

	SND_SEQ_EVENT_NONE = 255
};


typedef struct snd_seq_addr {
	unsigned char client;
	unsigned char port;
} snd_seq_addr_t;

typedef struct snd_seq_connect {
	snd_seq_addr_t sender;
	snd_seq_addr_t dest;
} snd_seq_connect_t;

typedef struct snd_seq_real_time {
	unsigned int tv_sec;
	unsigned int tv_nsec;
} snd_seq_real_time_t;

typedef unsigned int snd_seq_tick_time_t;

typedef union snd_seq_timestamp {
	snd_seq_tick_time_t tick;
	struct snd_seq_real_time time;
} snd_seq_timestamp_t;

/* Event mode flags */
#define SND_SEQ_TIME_STAMP_TICK		(0<<0)
#define SND_SEQ_TIME_STAMP_REAL		(1<<0)
#define SND_SEQ_TIME_STAMP_MASK		(1<<0)

#define SND_SEQ_TIME_MODE_ABS		(0<<1)
#define SND_SEQ_TIME_MODE_REL		(1<<1)
#define SND_SEQ_TIME_MODE_MASK		(1<<1)

#define SND_SEQ_EVENT_LENGTH_FIXED	(0<<2)
#define SND_SEQ_EVENT_LENGTH_VARIABLE	(1<<2)
#define SND_SEQ_EVENT_LENGTH_VARUSR	(2<<2)
#define SND_SEQ_EVENT_LENGTH_MASK	(3<<2)

#define SND_SEQ_PRIORITY_NORMAL		(0<<4)
#define SND_SEQ_PRIORITY_HIGH		(1<<4)
#define SND_SEQ_PRIORITY_MASK		(1<<4)

/* Note event */
typedef struct snd_seq_ev_note {
	unsigned char channel;
	unsigned char note;
	unsigned char velocity;
	unsigned char off_velocity;
	unsigned int duration;
} snd_seq_ev_note_t;

/* Controller event */
typedef struct snd_seq_ev_ctrl {
	unsigned char channel;
	unsigned char unused[3];
	unsigned int param;
	signed int value;
} snd_seq_ev_ctrl_t;

typedef struct snd_seq_ev_raw8 {
	unsigned char d[12];
} snd_seq_ev_raw8_t;

typedef struct snd_seq_ev_raw32 {
	unsigned int d[3];
} snd_seq_ev_raw32_t;

typedef struct snd_seq_ev_ext {
	unsigned int len;
	void *ptr;
} __attribute__((packed)) snd_seq_ev_ext_t;

/* Result events */
typedef struct snd_seq_result {
	int event;
	int result;
} snd_seq_result_t;

/* Queue skew values */
typedef struct snd_seq_queue_skew {
	unsigned int value;
	unsigned int base;
} snd_seq_queue_skew_t;

/* queue timer control */
typedef struct snd_seq_ev_queue_control {
	unsigned char queue;
	unsigned char unused[3];
	union {
		signed int value;
		snd_seq_timestamp_t time;
		unsigned int position;
		snd_seq_queue_skew_t skew;
		unsigned int d32[2];
		unsigned char d8[8];
	} param;
} snd_seq_ev_queue_control_t;

/* Sequencer event */
typedef struct snd_seq_event {
	snd_seq_event_type_t type;
	unsigned char flags;
	unsigned char tag;

	unsigned char queue;
	snd_seq_timestamp_t time;

	snd_seq_addr_t source;
	snd_seq_addr_t dest;

	union {
		snd_seq_ev_note_t note;
		snd_seq_ev_ctrl_t control;
		snd_seq_ev_raw8_t raw8;
		snd_seq_ev_raw32_t raw32;
		snd_seq_ev_ext_t ext;
		snd_seq_ev_queue_control_t queue;
		snd_seq_timestamp_t time;
		snd_seq_addr_t addr;
		snd_seq_connect_t connect;
		snd_seq_result_t result;
	} data;
} snd_seq_event_t;

#endif /* __ALSA_SEQ_EVENT_H */

