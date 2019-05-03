/*
	SGLUALSA.h

	Copyright (C) 2012 Stephan Kochen, Paul C. Pratt

	You can redistribute this file and/or modify it under the terms
	of version 2 of the GNU General Public License as published by
	the Free Software Foundation.  You should have received a copy
	of the license along with this file; see the file COPYING.

	This file is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	license for more details.
*/

/*
	Sound GLUe for ALSA

	ALSA sound support by Stephan Kochen.
*/

#ifndef RaspbianWorkAround
#define RaspbianWorkAround 0
#endif

#if 0

#include "alsa/asoundlib.h"
	/* and link with "-lasound" */

#define My_snd_pcm_t snd_pcm_t
#define My_snd_pcm_hw_params_t snd_pcm_hw_params_t
#define My_snd_pcm_sw_params_t snd_pcm_sw_params_t
#define My_snd_pcm_state_t snd_pcm_state_t
#if RaspbianWorkAround
#define My_snd_pcm_status_t snd_pcm_status_t
#endif

#define My_SND_PCM_STATE_OPEN SND_PCM_STATE_OPEN
#define My_SND_PCM_STATE_SETUP SND_PCM_STATE_SETUP
#define My_SND_PCM_STATE_PREPARED SND_PCM_STATE_PREPARED
#define My_SND_PCM_STATE_RUNNING SND_PCM_STATE_RUNNING
#define My_SND_PCM_STATE_XRUN SND_PCM_STATE_XRUN
#define My_SND_PCM_STATE_DRAINING SND_PCM_STATE_DRAINING
#define My_SND_PCM_STATE_PAUSED SND_PCM_STATE_PAUSED
#define My_SND_PCM_STATE_SUSPENDED SND_PCM_STATE_SUSPENDED
#define My_SND_PCM_STATE_DISCONNECTED SND_PCM_STATE_DISCONNECTED
#define My_SND_PCM_STATE_LAST SND_PCM_STATE_LAST

#define My_snd_pcm_stream_t snd_pcm_stream_t

#define My_SND_PCM_STREAM_PLAYBACK SND_PCM_STREAM_PLAYBACK
#define My_SND_PCM_STREAM_CAPTURE SND_PCM_STREAM_CAPTURE
#define My_SND_PCM_STREAM_LAST SND_PCM_STREAM_LAST

#define My_snd_pcm_access_t snd_pcm_access_t

#define My_SND_PCM_ACCESS_MMAP_INTERLEAVED \
	SND_PCM_ACCESS_MMAP_INTERLEAVED
#define My_SND_PCM_ACCESS_MMAP_NONINTERLEAVED \
	SND_PCM_ACCESS_MMAP_NONINTERLEAVED
#define My_SND_PCM_ACCESS_MMAP_COMPLEX \
	SND_PCM_ACCESS_MMAP_COMPLEX
#define My_SND_PCM_ACCESS_RW_INTERLEAVED \
	SND_PCM_ACCESS_RW_INTERLEAVED
#define My_SND_PCM_ACCESS_RW_NONINTERLEAVED \
	SND_PCM_ACCESS_RW_NONINTERLEAVED
#define My_SND_PCM_ACCESS_LAST \
	SND_PCM_ACCESS_LAST

#define My_snd_pcm_format_t snd_pcm_format_t

#define My_SND_PCM_FORMAT_UNKNOWN SND_PCM_FORMAT_UNKNOWN
#define My_SND_PCM_FORMAT_S8 SND_PCM_FORMAT_S8
#define My_SND_PCM_FORMAT_U8 SND_PCM_FORMAT_U8
#define My_SND_PCM_FORMAT_S16_LE SND_PCM_FORMAT_S16_LE
#define My_SND_PCM_FORMAT_S16_BE SND_PCM_FORMAT_S16_BE
#define My_SND_PCM_FORMAT_U16_LE SND_PCM_FORMAT_U16_LE
#define My_SND_PCM_FORMAT_U16_BE SND_PCM_FORMAT_U16_BE
#define My_SND_PCM_FORMAT_S24_LE SND_PCM_FORMAT_S24_LE
#define My_SND_PCM_FORMAT_S24_BE SND_PCM_FORMAT_S24_BE
#define My_SND_PCM_FORMAT_U24_LE SND_PCM_FORMAT_U24_LE
#define My_SND_PCM_FORMAT_U24_BE SND_PCM_FORMAT_U24_BE
#define My_SND_PCM_FORMAT_S32_LE SND_PCM_FORMAT_S32_LE
#define My_SND_PCM_FORMAT_S32_BE SND_PCM_FORMAT_S32_BE
#define My_SND_PCM_FORMAT_U32_LE SND_PCM_FORMAT_U32_LE
#define My_SND_PCM_FORMAT_U32_BE SND_PCM_FORMAT_U32_BE
#define My_SND_PCM_FORMAT_FLOAT_LE SND_PCM_FORMAT_FLOAT_LE
#define My_SND_PCM_FORMAT_FLOAT_BE SND_PCM_FORMAT_FLOAT_BE
#define My_SND_PCM_FORMAT_FLOAT64_LE SND_PCM_FORMAT_FLOAT64_LE
#define My_SND_PCM_FORMAT_FLOAT64_BE SND_PCM_FORMAT_FLOAT64_BE
#define My_SND_PCM_FORMAT_IEC958_SUBFRAME_LE \
	SND_PCM_FORMAT_IEC958_SUBFRAME_LE
#define My_SND_PCM_FORMAT_IEC958_SUBFRAME_BE \
	SND_PCM_FORMAT_IEC958_SUBFRAME_BE
#define My_SND_PCM_FORMAT_MU_LAW SND_PCM_FORMAT_MU_LAW
#define My_SND_PCM_FORMAT_A_LAW SND_PCM_FORMAT_A_LAW
#define My_SND_PCM_FORMAT_IMA_ADPCM SND_PCM_FORMAT_IMA_ADPCM
#define My_SND_PCM_FORMAT_MPEG SND_PCM_FORMAT_MPEG
#define My_SND_PCM_FORMAT_GSM SND_PCM_FORMAT_GSM
#define My_SND_PCM_FORMAT_SPECIAL SND_PCM_FORMAT_SPECIAL
#define My_SND_PCM_FORMAT_S24_3LE SND_PCM_FORMAT_S24_3LE
#define My_SND_PCM_FORMAT_S24_3BE SND_PCM_FORMAT_S24_3BE
#define My_SND_PCM_FORMAT_U24_3LE SND_PCM_FORMAT_U24_3LE
#define My_SND_PCM_FORMAT_U24_3BE SND_PCM_FORMAT_U24_3BE
#define My_SND_PCM_FORMAT_S20_3LE SND_PCM_FORMAT_S20_3LE
#define My_SND_PCM_FORMAT_S20_3BE SND_PCM_FORMAT_S20_3BE
#define My_SND_PCM_FORMAT_U20_3LE SND_PCM_FORMAT_U20_3LE
#define My_SND_PCM_FORMAT_U20_3BE SND_PCM_FORMAT_U20_3BE
#define My_SND_PCM_FORMAT_S18_3LE SND_PCM_FORMAT_S18_3LE
#define My_SND_PCM_FORMAT_S18_3BE SND_PCM_FORMAT_S18_3BE
#define My_SND_PCM_FORMAT_U18_3LE SND_PCM_FORMAT_U18_3LE
#define My_SND_PCM_FORMAT_U18_3BE SND_PCM_FORMAT_U18_3BE
#define My_SND_PCM_FORMAT_LAST SND_PCM_FORMAT_LAST

#define My_SND_PCM_FORMAT_S16 SND_PCM_FORMAT_S16
#define My_SND_PCM_FORMAT_U16 SND_PCM_FORMAT_U16
#define My_SND_PCM_FORMAT_S24 SND_PCM_FORMAT_S24
#define My_SND_PCM_FORMAT_U24 SND_PCM_FORMAT_U24
#define My_SND_PCM_FORMAT_S32 SND_PCM_FORMAT_S32
#define My_SND_PCM_FORMAT_U32 SND_PCM_FORMAT_U32
#define My_SND_PCM_FORMAT_FLOAT SND_PCM_FORMAT_FLOAT
#define My_SND_PCM_FORMAT_FLOAT64 SND_PCM_FORMAT_FLOAT64
#define My_SND_PCM_FORMAT_IEC958_SUBFRAME SND_PCM_FORMAT_FLOAT64

#define My_snd_pcm_uframes_t snd_pcm_uframes_t
#define My_snd_pcm_sframes_t snd_pcm_sframes_t

#define My_SND_PCM_NONBLOCK SND_PCM_NONBLOCK

#define My_snd_pcm_open snd_pcm_open
#define HaveMy_snd_pcm_open() (1)

#define My_snd_pcm_close snd_pcm_close
#define HaveMy_snd_pcm_close() (1)

#define My_snd_pcm_hw_params_malloc snd_pcm_hw_params_malloc
#define HaveMy_snd_pcm_hw_params_malloc() (1)

#define My_snd_pcm_hw_params_free snd_pcm_hw_params_free
#define HaveMy_snd_pcm_hw_params_free() (1)

#define My_snd_pcm_hw_params_any snd_pcm_hw_params_any
#define HaveMy_snd_pcm_hw_params_any() (1)

#define My_snd_pcm_hw_params_set_access snd_pcm_hw_params_set_access
#define HaveMy_snd_pcm_hw_params_set_access() (1)

#define My_snd_pcm_hw_params_set_format snd_pcm_hw_params_set_format
#define HaveMy_snd_pcm_hw_params_set_format() (1)

#define My_snd_pcm_hw_params_set_rate_near \
	snd_pcm_hw_params_set_rate_near
#define HaveMy_snd_pcm_hw_params_set_rate_near() (1)

#define My_snd_pcm_hw_params_set_channels \
	snd_pcm_hw_params_set_channels
#define HaveMy_snd_pcm_hw_params_set_channels() (1)

#define My_snd_pcm_hw_params_set_buffer_size_near \
	snd_pcm_hw_params_set_buffer_size_near
#define HaveMy_snd_pcm_hw_params_set_buffer_size_near() (1)

#define My_snd_pcm_hw_params_set_period_size_near \
	snd_pcm_hw_params_set_period_size_near
#define HaveMy_snd_pcm_hw_params_set_period_size_near() (1)

#define My_snd_pcm_hw_params snd_pcm_hw_params
#define HaveMy_snd_pcm_hw_params() (1)

#define My_snd_pcm_sw_params_malloc snd_pcm_sw_params_malloc
#define HaveMy_snd_pcm_sw_params_malloc() (1)

#define My_snd_pcm_sw_params_free snd_pcm_sw_params_free
#define HaveMy_snd_pcm_sw_params_free() (1)

#define My_snd_pcm_sw_params_current snd_pcm_sw_params_current
#define HaveMy_snd_pcm_sw_params_current() (1)

#define My_snd_pcm_sw_params_set_start_threshold \
	snd_pcm_sw_params_set_start_threshold
#define HaveMy_snd_pcm_sw_params_set_start_threshold() (1)

#define My_snd_pcm_sw_params_set_avail_min \
	snd_pcm_sw_params_set_avail_min
#define HaveMy_snd_pcm_sw_params_set_avail_min() (1)

#define My_snd_pcm_sw_params_set_xfer_align \
	snd_pcm_sw_params_set_xfer_align
#define HaveMy_snd_pcm_sw_params_set_xfer_align() (1)

#define My_snd_pcm_sw_params snd_pcm_sw_params
#define HaveMy_snd_pcm_sw_params() (1)

#define My_snd_pcm_nonblock snd_pcm_nonblock
#define HaveMy_snd_pcm_nonblock() (1)

#define My_snd_pcm_state snd_pcm_state
#define HaveMy_snd_pcm_state() (1)

#define My_snd_pcm_prepare snd_pcm_prepare
#define HaveMy_snd_pcm_prepare() (1)

#define My_snd_pcm_start snd_pcm_start
#define HaveMy_snd_pcm_start() (1)

#define My_snd_pcm_resume snd_pcm_resume
#define HaveMy_snd_pcm_resume() (1)

#define My_snd_pcm_avail_update snd_pcm_avail_update
#define HaveMy_snd_pcm_avail_update() (1)

#define My_snd_pcm_writei snd_pcm_writei
#define HaveMy_snd_pcm_writei() (1)

#define My_snd_pcm_drop snd_pcm_drop
#define HaveMy_snd_pcm_drop() (1)

#if RaspbianWorkAround
#define My_snd_pcm_status_malloc snd_pcm_status_malloc
#define HaveMy_snd_pcm_status_malloc() (1)

#define My_snd_pcm_status snd_pcm_status
#define HaveMy_snd_pcm_status() (1)

#define My_snd_pcm_status_get_avail snd_pcm_status_get_avail
#define HaveMy_snd_pcm_status_get_avail() (1)
#endif

#define My_snd_strerror snd_strerror
#define HaveMy_snd_strerror() (1)

#define MyCloseAlsaLib()

#else

static void *alsa_handle = NULL;

LOCALVAR blnr DidAlsaLib = falseblnr;

LOCALFUNC blnr HaveAlsaLib(void)
{
	if (! DidAlsaLib) {
		alsa_handle = dlopen("libasound.so.2", RTLD_NOW);
		if (NULL == alsa_handle) {
			fprintf(stderr, "dlopen libasound failed\n");
		}
		DidAlsaLib = trueblnr;
	}
	return (alsa_handle != NULL);
}

LOCALPROC MyCloseAlsaLib(void)
{
	if (NULL != alsa_handle) {
		if (0 != dlclose(alsa_handle)) {
			fprintf(stderr, "dlclose libasound failed\n");
		}
		alsa_handle = NULL;
	}
}

/* PCM handle */
typedef struct My__snd_pcm My_snd_pcm_t;
/* PCM hardware configuration space container */
typedef struct My__snd_pcm_hw_params My_snd_pcm_hw_params_t;
/* PCM software configuration container */
typedef struct My__snd_pcm_sw_params My_snd_pcm_sw_params_t;

#if RaspbianWorkAround
/* PCM status container */
typedef struct My__snd_pcm_status My_snd_pcm_status_t;
#endif

/* PCM state */
typedef enum My__snd_pcm_state {
	/* Open */
	My_SND_PCM_STATE_OPEN = 0,
	/* Setup installed */
	My_SND_PCM_STATE_SETUP,
	/* Ready to start */
	My_SND_PCM_STATE_PREPARED,
	/* Running */
	My_SND_PCM_STATE_RUNNING,
	/* Stopped: underrun (playback) or overrun (capture) detected */
	My_SND_PCM_STATE_XRUN,
	/* Draining: running (playback) or stopped (capture) */
	My_SND_PCM_STATE_DRAINING,
	/* Paused */
	My_SND_PCM_STATE_PAUSED,
	/* Hardware is suspended */
	My_SND_PCM_STATE_SUSPENDED,
	/* Hardware is disconnected */
	My_SND_PCM_STATE_DISCONNECTED,
	My_SND_PCM_STATE_LAST = My_SND_PCM_STATE_DISCONNECTED
} My_snd_pcm_state_t;

/* PCM stream (direction) */
typedef enum My__snd_pcm_stream {
	/* Playback stream */
	My_SND_PCM_STREAM_PLAYBACK = 0,
	/* Capture stream */
	My_SND_PCM_STREAM_CAPTURE,
	My_SND_PCM_STREAM_LAST = My_SND_PCM_STREAM_CAPTURE
} My_snd_pcm_stream_t;

/* PCM access type */
typedef enum My__snd_pcm_access {
	/* mmap access with simple interleaved channels */
	My_SND_PCM_ACCESS_MMAP_INTERLEAVED = 0,
	/* mmap access with simple non interleaved channels */
	My_SND_PCM_ACCESS_MMAP_NONINTERLEAVED,
	/* mmap access with complex placement */
	My_SND_PCM_ACCESS_MMAP_COMPLEX,
	/* snd_pcm_readi/snd_pcm_writei access */
	My_SND_PCM_ACCESS_RW_INTERLEAVED,
	/* snd_pcm_readn/snd_pcm_writen access */
	My_SND_PCM_ACCESS_RW_NONINTERLEAVED,
	My_SND_PCM_ACCESS_LAST = My_SND_PCM_ACCESS_RW_NONINTERLEAVED
} My_snd_pcm_access_t;

/* PCM sample format */
typedef enum My__snd_pcm_format {
	/* Unknown */
	My_SND_PCM_FORMAT_UNKNOWN = -1,
	/* Signed 8 bit */
	My_SND_PCM_FORMAT_S8 = 0,
	/* Unsigned 8 bit */
	My_SND_PCM_FORMAT_U8,
	/* Signed 16 bit Little Endian */
	My_SND_PCM_FORMAT_S16_LE,
	/* Signed 16 bit Big Endian */
	My_SND_PCM_FORMAT_S16_BE,
	/* Unsigned 16 bit Little Endian */
	My_SND_PCM_FORMAT_U16_LE,
	/* Unsigned 16 bit Big Endian */
	My_SND_PCM_FORMAT_U16_BE,
	/*
		Signed 24 bit Little Endian using low three bytes in 32-bit word
	*/
	My_SND_PCM_FORMAT_S24_LE,
	/* Signed 24 bit Big Endian using low three bytes in 32-bit word */
	My_SND_PCM_FORMAT_S24_BE,
	/*
		Unsigned 24 bit Little Endian using low three bytes in
		32-bit word
	*/
	My_SND_PCM_FORMAT_U24_LE,
	/*
		Unsigned 24 bit Big Endian using low three bytes in 32-bit word
	*/
	My_SND_PCM_FORMAT_U24_BE,
	/* Signed 32 bit Little Endian */
	My_SND_PCM_FORMAT_S32_LE,
	/* Signed 32 bit Big Endian */
	My_SND_PCM_FORMAT_S32_BE,
	/* Unsigned 32 bit Little Endian */
	My_SND_PCM_FORMAT_U32_LE,
	/* Unsigned 32 bit Big Endian */
	My_SND_PCM_FORMAT_U32_BE,
	/* Float 32 bit Little Endian, Range -1.0 to 1.0 */
	My_SND_PCM_FORMAT_FLOAT_LE,
	/* Float 32 bit Big Endian, Range -1.0 to 1.0 */
	My_SND_PCM_FORMAT_FLOAT_BE,
	/* Float 64 bit Little Endian, Range -1.0 to 1.0 */
	My_SND_PCM_FORMAT_FLOAT64_LE,
	/* Float 64 bit Big Endian, Range -1.0 to 1.0 */
	My_SND_PCM_FORMAT_FLOAT64_BE,
	/* IEC-958 Little Endian */
	My_SND_PCM_FORMAT_IEC958_SUBFRAME_LE,
	/* IEC-958 Big Endian */
	My_SND_PCM_FORMAT_IEC958_SUBFRAME_BE,
	/* Mu-Law */
	My_SND_PCM_FORMAT_MU_LAW,
	/* A-Law */
	My_SND_PCM_FORMAT_A_LAW,
	/* Ima-ADPCM */
	My_SND_PCM_FORMAT_IMA_ADPCM,
	/* MPEG */
	My_SND_PCM_FORMAT_MPEG,
	/* GSM */
	My_SND_PCM_FORMAT_GSM,
	/* Special */
	My_SND_PCM_FORMAT_SPECIAL = 31,
	/* Signed 24bit Little Endian in 3bytes format */
	My_SND_PCM_FORMAT_S24_3LE = 32,
	/* Signed 24bit Big Endian in 3bytes format */
	My_SND_PCM_FORMAT_S24_3BE,
	/* Unsigned 24bit Little Endian in 3bytes format */
	My_SND_PCM_FORMAT_U24_3LE,
	/* Unsigned 24bit Big Endian in 3bytes format */
	My_SND_PCM_FORMAT_U24_3BE,
	/* Signed 20bit Little Endian in 3bytes format */
	My_SND_PCM_FORMAT_S20_3LE,
	/* Signed 20bit Big Endian in 3bytes format */
	My_SND_PCM_FORMAT_S20_3BE,
	/* Unsigned 20bit Little Endian in 3bytes format */
	My_SND_PCM_FORMAT_U20_3LE,
	/* Unsigned 20bit Big Endian in 3bytes format */
	My_SND_PCM_FORMAT_U20_3BE,
	/* Signed 18bit Little Endian in 3bytes format */
	My_SND_PCM_FORMAT_S18_3LE,
	/* Signed 18bit Big Endian in 3bytes format */
	My_SND_PCM_FORMAT_S18_3BE,
	/* Unsigned 18bit Little Endian in 3bytes format */
	My_SND_PCM_FORMAT_U18_3LE,
	/* Unsigned 18bit Big Endian in 3bytes format */
	My_SND_PCM_FORMAT_U18_3BE,
	My_SND_PCM_FORMAT_LAST = My_SND_PCM_FORMAT_U18_3BE,

#if __BYTE_ORDER == __LITTLE_ENDIAN
	/* Signed 16 bit CPU endian */
	My_SND_PCM_FORMAT_S16 = My_SND_PCM_FORMAT_S16_LE,
	/* Unsigned 16 bit CPU endian */
	My_SND_PCM_FORMAT_U16 = My_SND_PCM_FORMAT_U16_LE,
	/* Signed 24 bit CPU endian */
	My_SND_PCM_FORMAT_S24 = My_SND_PCM_FORMAT_S24_LE,
	/* Unsigned 24 bit CPU endian */
	My_SND_PCM_FORMAT_U24 = My_SND_PCM_FORMAT_U24_LE,
	/* Signed 32 bit CPU endian */
	My_SND_PCM_FORMAT_S32 = My_SND_PCM_FORMAT_S32_LE,
	/* Unsigned 32 bit CPU endian */
	My_SND_PCM_FORMAT_U32 = My_SND_PCM_FORMAT_U32_LE,
	/* Float 32 bit CPU endian */
	My_SND_PCM_FORMAT_FLOAT = My_SND_PCM_FORMAT_FLOAT_LE,
	/* Float 64 bit CPU endian */
	My_SND_PCM_FORMAT_FLOAT64 = My_SND_PCM_FORMAT_FLOAT64_LE,
	/* IEC-958 CPU Endian */
	My_SND_PCM_FORMAT_IEC958_SUBFRAME =
		My_SND_PCM_FORMAT_IEC958_SUBFRAME_LE
#elif __BYTE_ORDER == __BIG_ENDIAN
	/* Signed 16 bit CPU endian */
	My_SND_PCM_FORMAT_S16 = My_SND_PCM_FORMAT_S16_BE,
	/* Unsigned 16 bit CPU endian */
	My_SND_PCM_FORMAT_U16 = My_SND_PCM_FORMAT_U16_BE,
	/* Signed 24 bit CPU endian */
	My_SND_PCM_FORMAT_S24 = My_SND_PCM_FORMAT_S24_BE,
	/* Unsigned 24 bit CPU endian */
	My_SND_PCM_FORMAT_U24 = My_SND_PCM_FORMAT_U24_BE,
	/* Signed 32 bit CPU endian */
	My_SND_PCM_FORMAT_S32 = My_SND_PCM_FORMAT_S32_BE,
	/* Unsigned 32 bit CPU endian */
	My_SND_PCM_FORMAT_U32 = My_SND_PCM_FORMAT_U32_BE,
	/* Float 32 bit CPU endian */
	My_SND_PCM_FORMAT_FLOAT = My_SND_PCM_FORMAT_FLOAT_BE,
	/* Float 64 bit CPU endian */
	My_SND_PCM_FORMAT_FLOAT64 = My_SND_PCM_FORMAT_FLOAT64_BE,
	/* IEC-958 CPU Endian */
	My_SND_PCM_FORMAT_IEC958_SUBFRAME =
		My_SND_PCM_FORMAT_IEC958_SUBFRAME_BE
#else
#error "Unknown endian"
#endif
} My_snd_pcm_format_t;

/* Unsigned frames quantity */
typedef unsigned long My_snd_pcm_uframes_t;
/* Signed frames quantity */
typedef long My_snd_pcm_sframes_t;

/* Non blocking mode (flag for open mode) \hideinitializer */
#define My_SND_PCM_NONBLOCK 0x00000001

typedef int (*snd_pcm_open_ProcPtr)
	(My_snd_pcm_t **pcm, const char *name, My_snd_pcm_stream_t stream,
		int mode);
LOCALVAR snd_pcm_open_ProcPtr My_snd_pcm_open = NULL;
LOCALVAR blnr Did_snd_pcm_open = falseblnr;

LOCALFUNC blnr HaveMy_snd_pcm_open(void)
{
	if (! Did_snd_pcm_open) {
		if (HaveAlsaLib()) {
			My_snd_pcm_open = (snd_pcm_open_ProcPtr)dlsym(alsa_handle,
				"snd_pcm_open");
			if (NULL == My_snd_pcm_open) {
				fprintf(stderr, "dlsym snd_pcm_open failed\n");
			}
		}
		Did_snd_pcm_open = trueblnr;
	}
	return (My_snd_pcm_open != NULL);
}

typedef int (*snd_pcm_close_ProcPtr)(My_snd_pcm_t *pcm);
LOCALVAR snd_pcm_close_ProcPtr My_snd_pcm_close = NULL;
LOCALVAR blnr Did_snd_pcm_close = falseblnr;

LOCALFUNC blnr HaveMy_snd_pcm_close(void)
{
	if (! Did_snd_pcm_close) {
		if (HaveAlsaLib()) {
			My_snd_pcm_close = (snd_pcm_close_ProcPtr)dlsym(alsa_handle,
				"snd_pcm_close");
			if (NULL == My_snd_pcm_close) {
				fprintf(stderr, "dlsym snd_pcm_close failed\n");
			}
		}
		Did_snd_pcm_close = trueblnr;
	}
	return (My_snd_pcm_close != NULL);
}

typedef int (*snd_pcm_hw_params_malloc_ProcPtr)
	(My_snd_pcm_hw_params_t **ptr);
LOCALVAR snd_pcm_hw_params_malloc_ProcPtr My_snd_pcm_hw_params_malloc
	= NULL;
LOCALVAR blnr Did_snd_pcm_hw_params_malloc = falseblnr;

LOCALFUNC blnr HaveMy_snd_pcm_hw_params_malloc(void)
{
	if (! Did_snd_pcm_hw_params_malloc) {
		if (HaveAlsaLib()) {
			My_snd_pcm_hw_params_malloc =
				(snd_pcm_hw_params_malloc_ProcPtr)
				dlsym(alsa_handle, "snd_pcm_hw_params_malloc");
			if (NULL == My_snd_pcm_hw_params_malloc) {
				fprintf(stderr,
					"dlsym snd_pcm_hw_params_malloc failed\n");
			}
		}
		Did_snd_pcm_hw_params_malloc = trueblnr;
	}
	return (My_snd_pcm_hw_params_malloc != NULL);
}

typedef void (*snd_pcm_hw_params_free_ProcPtr)
	(My_snd_pcm_hw_params_t *obj);
LOCALVAR snd_pcm_hw_params_free_ProcPtr
	My_snd_pcm_hw_params_free = NULL;
LOCALVAR blnr Did_snd_pcm_hw_params_free = falseblnr;

LOCALFUNC blnr HaveMy_snd_pcm_hw_params_free(void)
{
	if (! Did_snd_pcm_hw_params_free) {
		if (HaveAlsaLib()) {
			My_snd_pcm_hw_params_free = (snd_pcm_hw_params_free_ProcPtr)
				dlsym(alsa_handle, "snd_pcm_hw_params_free");
			if (NULL == My_snd_pcm_hw_params_free) {
				fprintf(stderr,
					"dlsym snd_pcm_hw_params_free failed\n");
			}
		}
		Did_snd_pcm_hw_params_free = trueblnr;
	}
	return (My_snd_pcm_hw_params_free != NULL);
}

typedef int (*snd_pcm_hw_params_any_ProcPtr)
	(My_snd_pcm_t *pcm, My_snd_pcm_hw_params_t *params);
LOCALVAR snd_pcm_hw_params_any_ProcPtr My_snd_pcm_hw_params_any = NULL;
LOCALVAR blnr Did_snd_pcm_hw_params_any = falseblnr;

LOCALFUNC blnr HaveMy_snd_pcm_hw_params_any(void)
{
	if (! Did_snd_pcm_hw_params_any) {
		if (HaveAlsaLib()) {
			My_snd_pcm_hw_params_any = (snd_pcm_hw_params_any_ProcPtr)
				dlsym(alsa_handle, "snd_pcm_hw_params_any");
			if (NULL == My_snd_pcm_hw_params_any) {
				fprintf(stderr, "dlsym snd_pcm_hw_params_any failed\n");
			}
		}
		Did_snd_pcm_hw_params_any = trueblnr;
	}
	return (My_snd_pcm_hw_params_any != NULL);
}

typedef int (*snd_pcm_hw_params_set_access_ProcPtr)
	(My_snd_pcm_t *pcm, My_snd_pcm_hw_params_t *params,
		My_snd_pcm_access_t _access);
LOCALVAR snd_pcm_hw_params_set_access_ProcPtr
	My_snd_pcm_hw_params_set_access = NULL;
LOCALVAR blnr Did_snd_pcm_hw_params_set_access = falseblnr;

LOCALFUNC blnr HaveMy_snd_pcm_hw_params_set_access(void)
{
	if (! Did_snd_pcm_hw_params_set_access) {
		if (HaveAlsaLib()) {
			My_snd_pcm_hw_params_set_access =
				(snd_pcm_hw_params_set_access_ProcPtr)
				dlsym(alsa_handle, "snd_pcm_hw_params_set_access");
			if (NULL == My_snd_pcm_hw_params_set_access) {
				fprintf(stderr,
					"dlsym snd_pcm_hw_params_set_access failed\n");
			}
		}
		Did_snd_pcm_hw_params_set_access = trueblnr;
	}
	return (My_snd_pcm_hw_params_set_access != NULL);
}

typedef int (*snd_pcm_hw_params_set_format_ProcPtr)
	(My_snd_pcm_t *pcm, My_snd_pcm_hw_params_t *params,
		My_snd_pcm_format_t val);
LOCALVAR snd_pcm_hw_params_set_format_ProcPtr
	My_snd_pcm_hw_params_set_format = NULL;
LOCALVAR blnr Did_snd_pcm_hw_params_set_format = falseblnr;

LOCALFUNC blnr HaveMy_snd_pcm_hw_params_set_format(void)
{
	if (! Did_snd_pcm_hw_params_set_format) {
		if (HaveAlsaLib()) {
			My_snd_pcm_hw_params_set_format =
				(snd_pcm_hw_params_set_format_ProcPtr)
				dlsym(alsa_handle, "snd_pcm_hw_params_set_format");
			if (NULL == My_snd_pcm_hw_params_set_format) {
				fprintf(stderr,
					"dlsym snd_pcm_hw_params_set_format failed\n");
			}
		}
		Did_snd_pcm_hw_params_set_format = trueblnr;
	}
	return (My_snd_pcm_hw_params_set_format != NULL);
}

typedef int (*snd_pcm_hw_params_set_rate_near_ProcPtr)
	(My_snd_pcm_t *pcm, My_snd_pcm_hw_params_t *params,
		unsigned int *val, int *dir);
LOCALVAR snd_pcm_hw_params_set_rate_near_ProcPtr
	My_snd_pcm_hw_params_set_rate_near = NULL;
LOCALVAR blnr Did_snd_pcm_hw_params_set_rate_near = falseblnr;

LOCALFUNC blnr HaveMy_snd_pcm_hw_params_set_rate_near(void)
{
	if (! Did_snd_pcm_hw_params_set_rate_near) {
		if (HaveAlsaLib()) {
			My_snd_pcm_hw_params_set_rate_near =
				(snd_pcm_hw_params_set_rate_near_ProcPtr)
				dlsym(alsa_handle, "snd_pcm_hw_params_set_rate_near");
			if (NULL == My_snd_pcm_hw_params_set_rate_near) {
				fprintf(stderr,
					"dlsym snd_pcm_hw_params_set_rate_near failed\n");
			}
		}
		Did_snd_pcm_hw_params_set_rate_near = trueblnr;
	}
	return (My_snd_pcm_hw_params_set_rate_near != NULL);
}

typedef int (*snd_pcm_hw_params_set_channels_ProcPtr)
	(My_snd_pcm_t *pcm, My_snd_pcm_hw_params_t *params,
	unsigned int val);
LOCALVAR snd_pcm_hw_params_set_channels_ProcPtr
	My_snd_pcm_hw_params_set_channels = NULL;
LOCALVAR blnr Did_snd_pcm_hw_params_set_channels = falseblnr;

LOCALFUNC blnr HaveMy_snd_pcm_hw_params_set_channels(void)
{
	if (! Did_snd_pcm_hw_params_set_channels) {
		if (HaveAlsaLib()) {
			My_snd_pcm_hw_params_set_channels =
				(snd_pcm_hw_params_set_channels_ProcPtr)
				dlsym(alsa_handle, "snd_pcm_hw_params_set_channels");
			if (NULL == My_snd_pcm_hw_params_set_channels) {
				fprintf(stderr,
					"dlsym snd_pcm_hw_params_set_channels failed\n");
			}
		}
		Did_snd_pcm_hw_params_set_channels = trueblnr;
	}
	return (My_snd_pcm_hw_params_set_channels != NULL);
}

typedef int (*snd_pcm_hw_params_set_buffer_size_near_ProcPtr)
	(My_snd_pcm_t *pcm, My_snd_pcm_hw_params_t *params,
		My_snd_pcm_uframes_t *val);
LOCALVAR snd_pcm_hw_params_set_buffer_size_near_ProcPtr
	My_snd_pcm_hw_params_set_buffer_size_near = NULL;
LOCALVAR blnr Did_snd_pcm_hw_params_set_buffer_size_near = falseblnr;

LOCALFUNC blnr HaveMy_snd_pcm_hw_params_set_buffer_size_near(void)
{
	if (! Did_snd_pcm_hw_params_set_buffer_size_near) {
		if (HaveAlsaLib()) {
			My_snd_pcm_hw_params_set_buffer_size_near =
				(snd_pcm_hw_params_set_buffer_size_near_ProcPtr)
				dlsym(alsa_handle,
					"snd_pcm_hw_params_set_buffer_size_near");
			if (NULL == My_snd_pcm_hw_params_set_buffer_size_near) {
				fprintf(stderr,
					"dlsym snd_pcm_hw_params_set_buffer_size_near"
					" failed\n");
			}
		}
		Did_snd_pcm_hw_params_set_buffer_size_near = trueblnr;
	}
	return (My_snd_pcm_hw_params_set_buffer_size_near != NULL);
}

typedef int (*snd_pcm_hw_params_set_period_size_near_ProcPtr)
	(My_snd_pcm_t *pcm, My_snd_pcm_hw_params_t *params,
		My_snd_pcm_uframes_t *val, int *dir);
LOCALVAR snd_pcm_hw_params_set_period_size_near_ProcPtr
	My_snd_pcm_hw_params_set_period_size_near = NULL;
LOCALVAR blnr Did_snd_pcm_hw_params_set_period_size_near = falseblnr;

LOCALFUNC blnr HaveMy_snd_pcm_hw_params_set_period_size_near(void)
{
	if (! Did_snd_pcm_hw_params_set_period_size_near) {
		if (HaveAlsaLib()) {
			My_snd_pcm_hw_params_set_period_size_near =
				(snd_pcm_hw_params_set_period_size_near_ProcPtr)
				dlsym(alsa_handle,
					"snd_pcm_hw_params_set_period_size_near");
			if (NULL == My_snd_pcm_hw_params_set_period_size_near) {
				fprintf(stderr,
					"dlsym snd_pcm_hw_params_set_period_size_near"
					" failed\n");
			}
		}
		Did_snd_pcm_hw_params_set_period_size_near = trueblnr;
	}
	return (My_snd_pcm_hw_params_set_period_size_near != NULL);
}

typedef int (*snd_pcm_hw_params_ProcPtr)
	(My_snd_pcm_t *pcm, My_snd_pcm_hw_params_t *params);
LOCALVAR snd_pcm_hw_params_ProcPtr My_snd_pcm_hw_params = NULL;
LOCALVAR blnr Did_snd_pcm_hw_params = falseblnr;

LOCALFUNC blnr HaveMy_snd_pcm_hw_params(void)
{
	if (! Did_snd_pcm_hw_params) {
		if (HaveAlsaLib()) {
			My_snd_pcm_hw_params = (snd_pcm_hw_params_ProcPtr)
				dlsym(alsa_handle, "snd_pcm_hw_params");
			if (NULL == My_snd_pcm_hw_params) {
				fprintf(stderr, "dlsym snd_pcm_hw_params failed\n");
			}
		}
		Did_snd_pcm_hw_params = trueblnr;
	}
	return (My_snd_pcm_hw_params != NULL);
}

typedef int (*snd_pcm_sw_params_malloc_ProcPtr)
	(My_snd_pcm_sw_params_t **ptr);
LOCALVAR snd_pcm_sw_params_malloc_ProcPtr
	My_snd_pcm_sw_params_malloc = NULL;
LOCALVAR blnr Did_snd_pcm_sw_params_malloc = falseblnr;

LOCALFUNC blnr HaveMy_snd_pcm_sw_params_malloc(void)
{
	if (! Did_snd_pcm_sw_params_malloc) {
		if (HaveAlsaLib()) {
			My_snd_pcm_sw_params_malloc =
				(snd_pcm_sw_params_malloc_ProcPtr)
				dlsym(alsa_handle, "snd_pcm_sw_params_malloc");
			if (NULL == My_snd_pcm_sw_params_malloc) {
				fprintf(stderr,
					"dlsym snd_pcm_sw_params_malloc failed\n");
			}
		}
		Did_snd_pcm_sw_params_malloc = trueblnr;
	}
	return (My_snd_pcm_sw_params_malloc != NULL);
}

typedef void (*snd_pcm_sw_params_free_ProcPtr)
	(My_snd_pcm_sw_params_t *obj);
LOCALVAR snd_pcm_sw_params_free_ProcPtr
	My_snd_pcm_sw_params_free = NULL;
LOCALVAR blnr Did_snd_pcm_sw_params_free = falseblnr;

LOCALFUNC blnr HaveMy_snd_pcm_sw_params_free(void)
{
	if (! Did_snd_pcm_sw_params_free) {
		if (HaveAlsaLib()) {
			My_snd_pcm_sw_params_free = (snd_pcm_sw_params_free_ProcPtr)
				dlsym(alsa_handle, "snd_pcm_sw_params_free");
			if (NULL == My_snd_pcm_sw_params_free) {
				fprintf(stderr,
					"dlsym snd_pcm_sw_params_free failed\n");
			}
		}
		Did_snd_pcm_sw_params_free = trueblnr;
	}
	return (My_snd_pcm_sw_params_free != NULL);
}

typedef int (*snd_pcm_sw_params_current_ProcPtr)
	(My_snd_pcm_t *pcm, My_snd_pcm_sw_params_t *params);
LOCALVAR snd_pcm_sw_params_current_ProcPtr
	My_snd_pcm_sw_params_current = NULL;
LOCALVAR blnr Did_snd_pcm_sw_params_current = falseblnr;

LOCALFUNC blnr HaveMy_snd_pcm_sw_params_current(void)
{
	if (! Did_snd_pcm_sw_params_current) {
		if (HaveAlsaLib()) {
			My_snd_pcm_sw_params_current =
				(snd_pcm_sw_params_current_ProcPtr)
				dlsym(alsa_handle, "snd_pcm_sw_params_current");
			if (NULL == My_snd_pcm_sw_params_current) {
				fprintf(stderr,
					"dlsym snd_pcm_sw_params_current failed\n");
			}
		}
		Did_snd_pcm_sw_params_current = trueblnr;
	}
	return (My_snd_pcm_sw_params_current != NULL);
}

typedef int (*snd_pcm_sw_params_set_start_threshold_ProcPtr)
	(My_snd_pcm_t *pcm, My_snd_pcm_sw_params_t *params,
		My_snd_pcm_uframes_t val);
LOCALVAR snd_pcm_sw_params_set_start_threshold_ProcPtr
	My_snd_pcm_sw_params_set_start_threshold = NULL;
LOCALVAR blnr Did_snd_pcm_sw_params_set_start_threshold = falseblnr;

LOCALFUNC blnr HaveMy_snd_pcm_sw_params_set_start_threshold(void)
{
	if (! Did_snd_pcm_sw_params_set_start_threshold) {
		if (HaveAlsaLib()) {
			My_snd_pcm_sw_params_set_start_threshold =
				(snd_pcm_sw_params_set_start_threshold_ProcPtr)
				dlsym(alsa_handle,
					"snd_pcm_sw_params_set_start_threshold");
			if (NULL == My_snd_pcm_sw_params_set_start_threshold) {
				fprintf(stderr,
					"dlsym snd_pcm_sw_params_set_start_threshold"
					" failed\n");
			}
		}
		Did_snd_pcm_sw_params_set_start_threshold = trueblnr;
	}
	return (My_snd_pcm_sw_params_set_start_threshold != NULL);
}

typedef int (*snd_pcm_sw_params_set_avail_min_ProcPtr)
	(My_snd_pcm_t *pcm, My_snd_pcm_sw_params_t *params,
		My_snd_pcm_uframes_t val);
LOCALVAR snd_pcm_sw_params_set_avail_min_ProcPtr
	My_snd_pcm_sw_params_set_avail_min = NULL;
LOCALVAR blnr Did_snd_pcm_sw_params_set_avail_min = falseblnr;

LOCALFUNC blnr HaveMy_snd_pcm_sw_params_set_avail_min(void)
{
	if (! Did_snd_pcm_sw_params_set_avail_min) {
		if (HaveAlsaLib()) {
			My_snd_pcm_sw_params_set_avail_min =
				(snd_pcm_sw_params_set_avail_min_ProcPtr)
				dlsym(alsa_handle, "snd_pcm_sw_params_set_avail_min");
			if (NULL == My_snd_pcm_sw_params_set_avail_min) {
				fprintf(stderr,
					"dlsym snd_pcm_sw_params_set_avail_min failed\n");
			}
		}
		Did_snd_pcm_sw_params_set_avail_min = trueblnr;
	}
	return (My_snd_pcm_sw_params_set_avail_min != NULL);
}

typedef int (*snd_pcm_sw_params_set_xfer_align_ProcPtr)
	(My_snd_pcm_t *pcm, My_snd_pcm_sw_params_t *params,
		My_snd_pcm_uframes_t val);
LOCALVAR snd_pcm_sw_params_set_xfer_align_ProcPtr
	My_snd_pcm_sw_params_set_xfer_align = NULL;
LOCALVAR blnr Did_snd_pcm_sw_params_set_xfer_align = falseblnr;

LOCALFUNC blnr HaveMy_snd_pcm_sw_params_set_xfer_align(void)
{
	if (! Did_snd_pcm_sw_params_set_xfer_align) {
		if (HaveAlsaLib()) {
			My_snd_pcm_sw_params_set_xfer_align =
				(snd_pcm_sw_params_set_xfer_align_ProcPtr)
				dlsym(alsa_handle, "snd_pcm_sw_params_set_xfer_align");
			if (NULL == My_snd_pcm_sw_params_set_xfer_align) {
				fprintf(stderr,
					"dlsym snd_pcm_sw_params_set_xfer_align failed\n");
			}
		}
		Did_snd_pcm_sw_params_set_xfer_align = trueblnr;
	}
	return (My_snd_pcm_sw_params_set_xfer_align != NULL);
}

typedef int (*snd_pcm_sw_params_ProcPtr)
	(My_snd_pcm_t *pcm, My_snd_pcm_sw_params_t *params);
LOCALVAR snd_pcm_sw_params_ProcPtr My_snd_pcm_sw_params = NULL;
LOCALVAR blnr Did_snd_pcm_sw_params = falseblnr;

LOCALFUNC blnr HaveMy_snd_pcm_sw_params(void)
{
	if (! Did_snd_pcm_sw_params) {
		if (HaveAlsaLib()) {
			My_snd_pcm_sw_params = (snd_pcm_sw_params_ProcPtr)
				dlsym(alsa_handle, "snd_pcm_sw_params");
			if (NULL == My_snd_pcm_sw_params) {
				fprintf(stderr, "dlsym snd_pcm_sw_params failed\n");
			}
		}
		Did_snd_pcm_sw_params = trueblnr;
	}
	return (My_snd_pcm_sw_params != NULL);
}

typedef int (*snd_pcm_nonblock_ProcPtr)
	(My_snd_pcm_t *pcm, int nonblock);
LOCALVAR snd_pcm_nonblock_ProcPtr My_snd_pcm_nonblock = NULL;
LOCALVAR blnr Did_snd_pcm_nonblock = falseblnr;

LOCALFUNC blnr HaveMy_snd_pcm_nonblock(void)
{
	if (! Did_snd_pcm_nonblock) {
		if (HaveAlsaLib()) {
			My_snd_pcm_nonblock = (snd_pcm_nonblock_ProcPtr)
				dlsym(alsa_handle, "snd_pcm_nonblock");
			if (NULL == My_snd_pcm_nonblock) {
				fprintf(stderr, "dlsym snd_pcm_nonblock failed\n");
			}
		}
		Did_snd_pcm_nonblock = trueblnr;
	}
	return (My_snd_pcm_nonblock != NULL);
}

typedef My_snd_pcm_state_t (*snd_pcm_state_ProcPtr)(My_snd_pcm_t *pcm);
LOCALVAR snd_pcm_state_ProcPtr My_snd_pcm_state = NULL;
LOCALVAR blnr Did_snd_pcm_state = falseblnr;

LOCALFUNC blnr HaveMy_snd_pcm_state(void)
{
	if (! Did_snd_pcm_state) {
		if (HaveAlsaLib()) {
			My_snd_pcm_state = (snd_pcm_state_ProcPtr)
				dlsym(alsa_handle, "snd_pcm_state");
			if (NULL == My_snd_pcm_state) {
				fprintf(stderr, "dlsym snd_pcm_state failed\n");
			}
		}
		Did_snd_pcm_state = trueblnr;
	}
	return (My_snd_pcm_state != NULL);
}

typedef int (*snd_pcm_prepare_ProcPtr)(My_snd_pcm_t *pcm);
LOCALVAR snd_pcm_prepare_ProcPtr My_snd_pcm_prepare = NULL;
LOCALVAR blnr Did_snd_pcm_prepare = falseblnr;

LOCALFUNC blnr HaveMy_snd_pcm_prepare(void)
{
	if (! Did_snd_pcm_prepare) {
		if (HaveAlsaLib()) {
			My_snd_pcm_prepare = (snd_pcm_prepare_ProcPtr)
				dlsym(alsa_handle, "snd_pcm_prepare");
			if (NULL == My_snd_pcm_prepare) {
				fprintf(stderr, "dlsym snd_pcm_prepare failed\n");
			}
		}
		Did_snd_pcm_prepare = trueblnr;
	}
	return (My_snd_pcm_prepare != NULL);
}

typedef int (*snd_pcm_start_ProcPtr)(My_snd_pcm_t *pcm);
LOCALVAR snd_pcm_start_ProcPtr My_snd_pcm_start = NULL;
LOCALVAR blnr Did_snd_pcm_start = falseblnr;

LOCALFUNC blnr HaveMy_snd_pcm_start(void)
{
	if (! Did_snd_pcm_start) {
		if (HaveAlsaLib()) {
			My_snd_pcm_start = (snd_pcm_start_ProcPtr)
				dlsym(alsa_handle, "snd_pcm_start");
			if (NULL == My_snd_pcm_start) {
				fprintf(stderr, "dlsym snd_pcm_start failed\n");
			}
		}
		Did_snd_pcm_start = trueblnr;
	}
	return (My_snd_pcm_start != NULL);
}

typedef int (*snd_pcm_resume_ProcPtr)(My_snd_pcm_t *pcm);
LOCALVAR snd_pcm_resume_ProcPtr My_snd_pcm_resume = NULL;
LOCALVAR blnr Did_snd_pcm_resume = falseblnr;

LOCALFUNC blnr HaveMy_snd_pcm_resume(void)
{
	if (! Did_snd_pcm_resume) {
		if (HaveAlsaLib()) {
			My_snd_pcm_resume = (snd_pcm_resume_ProcPtr)
				dlsym(alsa_handle, "snd_pcm_resume");
			if (NULL == My_snd_pcm_resume) {
				fprintf(stderr, "dlsym snd_pcm_resume failed\n");
			}
		}
		Did_snd_pcm_resume = trueblnr;
	}
	return (My_snd_pcm_resume != NULL);
}

typedef My_snd_pcm_sframes_t (*snd_pcm_avail_update_ProcPtr)
	(My_snd_pcm_t *pcm);
LOCALVAR snd_pcm_avail_update_ProcPtr My_snd_pcm_avail_update = NULL;
LOCALVAR blnr Did_snd_pcm_avail_update = falseblnr;

LOCALFUNC blnr HaveMy_snd_pcm_avail_update(void)
{
	if (! Did_snd_pcm_avail_update) {
		if (HaveAlsaLib()) {
			My_snd_pcm_avail_update = (snd_pcm_avail_update_ProcPtr)
				dlsym(alsa_handle, "snd_pcm_avail_update");
			if (NULL == My_snd_pcm_avail_update) {
				fprintf(stderr, "dlsym snd_pcm_avail_update failed\n");
			}
		}
		Did_snd_pcm_avail_update = trueblnr;
	}
	return (My_snd_pcm_avail_update != NULL);
}

typedef My_snd_pcm_sframes_t (*snd_pcm_writei_ProcPtr)
	(My_snd_pcm_t *pcm, const void *buffer, My_snd_pcm_uframes_t size);
LOCALVAR snd_pcm_writei_ProcPtr My_snd_pcm_writei = NULL;
LOCALVAR blnr Did_snd_pcm_writei = falseblnr;

LOCALFUNC blnr HaveMy_snd_pcm_writei(void)
{
	if (! Did_snd_pcm_writei) {
		if (HaveAlsaLib()) {
			My_snd_pcm_writei = (snd_pcm_writei_ProcPtr)
				dlsym(alsa_handle, "snd_pcm_writei");
			if (NULL == My_snd_pcm_writei) {
				fprintf(stderr, "dlsym snd_pcm_writei failed\n");
			}
		}
		Did_snd_pcm_writei = trueblnr;
	}
	return (My_snd_pcm_writei != NULL);
}

typedef int (*snd_pcm_drop_ProcPtr)(My_snd_pcm_t *pcm);
LOCALVAR snd_pcm_drop_ProcPtr My_snd_pcm_drop = NULL;
LOCALVAR blnr Did_snd_pcm_drop = falseblnr;

LOCALFUNC blnr HaveMy_snd_pcm_drop(void)
{
	if (! Did_snd_pcm_drop) {
		if (HaveAlsaLib()) {
			My_snd_pcm_drop = (snd_pcm_drop_ProcPtr)
				dlsym(alsa_handle, "snd_pcm_drop");
			if (NULL == My_snd_pcm_drop) {
				fprintf(stderr, "dlsym snd_pcm_drop failed\n");
			}
		}
		Did_snd_pcm_drop = trueblnr;
	}
	return (My_snd_pcm_drop != NULL);
}

#if RaspbianWorkAround
typedef int (*snd_pcm_status_malloc_ProcPtr)
	(My_snd_pcm_status_t **ptr);
LOCALVAR snd_pcm_status_malloc_ProcPtr My_snd_pcm_status_malloc = NULL;
LOCALVAR blnr Did_snd_pcm_status_malloc = falseblnr;

LOCALFUNC blnr HaveMy_snd_pcm_status_malloc(void)
{
	if (! Did_snd_pcm_status_malloc) {
		if (HaveAlsaLib()) {
			My_snd_pcm_status_malloc = (snd_pcm_status_malloc_ProcPtr)
				dlsym(alsa_handle, "snd_pcm_status_malloc");
			if (NULL == My_snd_pcm_status_malloc) {
				fprintf(stderr, "dlsym snd_pcm_status_malloc failed\n");
			}
		}
		Did_snd_pcm_status_malloc = trueblnr;
	}
	return (My_snd_pcm_status_malloc != NULL);
}
#endif

#if RaspbianWorkAround
typedef int (*snd_pcm_status_ProcPtr)(My_snd_pcm_t *pcm,
	My_snd_pcm_status_t *status);
LOCALVAR snd_pcm_status_ProcPtr My_snd_pcm_status = NULL;
LOCALVAR blnr Did_snd_pcm_status = falseblnr;

LOCALFUNC blnr HaveMy_snd_pcm_status(void)
{
	if (! Did_snd_pcm_status) {
		if (HaveAlsaLib()) {
			My_snd_pcm_status = (snd_pcm_status_ProcPtr)
				dlsym(alsa_handle, "snd_pcm_status");
			if (NULL == My_snd_pcm_status) {
				fprintf(stderr, "dlsym snd_pcm_status failed\n");
			}
		}
		Did_snd_pcm_status = trueblnr;
	}
	return (My_snd_pcm_status != NULL);
}
#endif

#if RaspbianWorkAround
typedef My_snd_pcm_uframes_t (*snd_pcm_status_get_avail_ProcPtr)
	(const My_snd_pcm_status_t *obj);
LOCALVAR snd_pcm_status_get_avail_ProcPtr
	My_snd_pcm_status_get_avail = NULL;
LOCALVAR blnr Did_snd_pcm_status_get_avail = falseblnr;

LOCALFUNC blnr HaveMy_snd_pcm_status_get_avail(void)
{
	if (! Did_snd_pcm_status_get_avail) {
		if (HaveAlsaLib()) {
			My_snd_pcm_status_get_avail =
				(snd_pcm_status_get_avail_ProcPtr)
				dlsym(alsa_handle, "snd_pcm_status_get_avail");
			if (NULL == My_snd_pcm_status_get_avail) {
				fprintf(stderr,
					"dlsym snd_pcm_status_get_avail failed\n");
			}
		}
		Did_snd_pcm_status_get_avail = trueblnr;
	}
	return (My_snd_pcm_status_get_avail != NULL);
}
#endif

typedef const char * (*snd_strerror_ProcPtr)(int errnum);
LOCALVAR snd_strerror_ProcPtr My_snd_strerror = NULL;
LOCALVAR blnr Did_snd_strerror = falseblnr;

LOCALFUNC blnr HaveMy_snd_strerror(void)
{
	if (! Did_snd_strerror) {
		if (HaveAlsaLib()) {
			My_snd_strerror = (snd_strerror_ProcPtr)
				dlsym(alsa_handle, "snd_strerror");
			if (NULL == My_snd_strerror) {
				fprintf(stderr, "dlsym snd_strerror failed\n");
			}
		}
		Did_snd_strerror = trueblnr;
	}
	return (My_snd_strerror != NULL);
}

#endif


/*
	The elaborate private buffer is mostly
	redundant since alsa has its own ring
	buffer. But using it keeps the code
	closer to the other ports. And anyway
	there is no guarantee just what size
	buffer you'll get from alsa.
*/


#if 4 == kLn2SoundSampSz
LOCALPROC ConvertSoundBlockToNative(tpSoundSamp p)
{
	int i;

	for (i = kOneBuffLen; --i >= 0; ) {
		*p++ -= 0x8000;
	}
}
#else
#define ConvertSoundBlockToNative(p)
#endif

#define desired_alsa_buffer_size kAllBuffLen
#define desired_alsa_period_size kOneBuffLen

LOCALVAR char *alsadev_name = NULL;

LOCALVAR My_snd_pcm_t *pcm_handle = NULL;
LOCALVAR My_snd_pcm_uframes_t buffer_size;
LOCALVAR My_snd_pcm_uframes_t period_size;


LOCALVAR blnr MySound_StartPend = falseblnr;

#if RaspbianWorkAround
LOCALVAR My_snd_pcm_status_t *my_status = NULL;

LOCALFUNC blnr HaveMyStatusAlloc(void)
{
	if (NULL == my_status) {
		if (HaveMy_snd_pcm_status_malloc())
		if (HaveMy_snd_pcm_status())
		if (HaveMy_snd_pcm_status_get_avail())
		{
			if (My_snd_pcm_status_malloc(&my_status) < 0) {
				my_status = NULL; /* just to make sure */
			} else {
				/* snd_pcm_status_free(my_status); */
			}
		}
	}

	return NULL != my_status;
}
#endif

LOCALPROC MySound_WriteOut(void)
{
	int retry_count = 32;

label_retry:
	if (--retry_count > 0) {
		My_snd_pcm_sframes_t avail;
		int err;
		My_snd_pcm_state_t cur_state = My_snd_pcm_state(pcm_handle);

		if (My_SND_PCM_STATE_PREPARED == cur_state) {
			if (! MySound_StartPend) {
				if (TheFillOffset - ThePlayOffset >= kAllBuffLen) {
					MySound_StartPend = trueblnr;
				}
			}
			if (MySound_StartPend) {
				cur_state = My_SND_PCM_STATE_RUNNING;
			}
		}

		if (My_SND_PCM_STATE_RUNNING != cur_state) {
			switch (cur_state) {
				case My_SND_PCM_STATE_SETUP:
				case My_SND_PCM_STATE_XRUN:
					err = My_snd_pcm_prepare(pcm_handle);
					if (err < 0) {
						fprintf(stderr, "pcm prepare error: %s\n",
							My_snd_strerror(err));
					} else {
						/* fprintf(stderr, "prepare succeeded\n"); */
						goto label_retry;
					}
					break;
				case My_SND_PCM_STATE_SUSPENDED:
					err = My_snd_pcm_resume(pcm_handle);
					if (err < 0) {
						fprintf(stderr, "pcm resume error: %s\n",
							My_snd_strerror(err));
					} else {
						/* fprintf(stderr, "resume succeeded\n"); */
						goto label_retry;
					}
					break;
				case My_SND_PCM_STATE_DISCONNECTED:
					/* just abort ? */
					break;
				case My_SND_PCM_STATE_PREPARED:
					/* leave */
					break;
				default:
					fprintf(stderr, "unknown alsa pcm state\n");
					break;
			}
		} else if ((avail = My_snd_pcm_avail_update(pcm_handle)) < 0) {
			fprintf(stderr, "pcm update error: %s\n",
				My_snd_strerror(avail));
		} else {
			tpSoundSamp NextPlayPtr;
			ui4b PlayNowSize = 0;
			ui4b MaskedFillOffset = ThePlayOffset & kOneBuffMask;

#if RaspbianWorkAround
			if ((avail > buffer_size) || (avail < 0)) {
				/*
					fprintf(stderr, "need avail workaround: %d\n",
						(int)avail);
				*/
				/* work around bug observed in Raspbian */
				if (HaveMyStatusAlloc()) {
					if (My_snd_pcm_status(pcm_handle, my_status) >= 0) {
						avail = My_snd_pcm_status_get_avail(my_status);
					}
				}
			}
#endif

			if (! MySound_StartPend) {
				My_snd_pcm_uframes_t used = buffer_size - avail;
				ui4b TotPendBuffs = used >> kLnOneBuffLen;

				if (TotPendBuffs < MinFilledSoundBuffs) {
					MinFilledSoundBuffs = TotPendBuffs;
				}
				/* fprintf(stderr, "buffer used %d\n", (int)used); */
			}

			if (MaskedFillOffset != 0) {
				/* take care of left overs */
				PlayNowSize = kOneBuffLen - MaskedFillOffset;
				NextPlayPtr =
					TheSoundBuffer + (ThePlayOffset & kAllBuffMask);
			} else if (0 !=
				((TheFillOffset - ThePlayOffset) >> kLnOneBuffLen))
			{
				PlayNowSize = kOneBuffLen;
				NextPlayPtr =
					TheSoundBuffer + (ThePlayOffset & kAllBuffMask);
			} else {
				/* nothing to play now */
			}

			if (PlayNowSize > avail) {
				/*
					This isn't supposed to be needed with nonblock
					mode. But in Ubuntu 7.04 running in Parallels,
					snd_pcm_writei seemed to block anyway.
				*/
				PlayNowSize = avail;
			}

			if (0 != PlayNowSize) {
				err = My_snd_pcm_writei(
					pcm_handle, NextPlayPtr, PlayNowSize);
				if (err < 0) {
					if ((- EAGAIN == err) || (- ESTRPIPE == err)) {
						/* buffer full, try again later */
						/* fprintf(stderr, "pcm write: EAGAIN\n"); */
					} else if (- EPIPE == err) {
						/* buffer seems to have emptied */
						/* fprintf(stderr, "pcm write emptied\n"); */
						goto label_retry;
					} else {
						fprintf(stderr, "pcm write error: %s\n",
							My_snd_strerror(err));
					}
				} else {
					ThePlayOffset += err;
					goto label_retry;
				}
			} else if (MySound_StartPend) {
				MySound_StartPend = falseblnr;
				if ((err = My_snd_pcm_start(pcm_handle)) < 0) {
					fprintf(stderr, "pcm start error: %s\n",
						My_snd_strerror(err));
				}
			}
		}
	}
}

LOCALPROC MySound_Start(void)
{
	if (pcm_handle != NULL) {
		MySound_Start0();
	}
}

LOCALPROC MySound_Stop(void)
{
	if (pcm_handle != NULL) {
		My_snd_pcm_drop(pcm_handle);
	}
}

LOCALFUNC blnr HaveAlsaRoutines(void)
{
	blnr IsOk = falseblnr;

	if (HaveMy_snd_pcm_open())
	if (HaveMy_snd_pcm_close())
	if (HaveMy_snd_pcm_hw_params_malloc())
	if (HaveMy_snd_pcm_hw_params_free())
	if (HaveMy_snd_pcm_hw_params_any())
	if (HaveMy_snd_pcm_hw_params_set_access())
	if (HaveMy_snd_pcm_hw_params_set_format())
	if (HaveMy_snd_pcm_hw_params_set_rate_near())
	if (HaveMy_snd_pcm_hw_params_set_channels())
	if (HaveMy_snd_pcm_hw_params_set_buffer_size_near())
	if (HaveMy_snd_pcm_hw_params_set_period_size_near())
	if (HaveMy_snd_pcm_hw_params())
	if (HaveMy_snd_pcm_sw_params_malloc())
	if (HaveMy_snd_pcm_sw_params_free())
	if (HaveMy_snd_pcm_sw_params_current())
	if (HaveMy_snd_pcm_sw_params_set_start_threshold())
	if (HaveMy_snd_pcm_sw_params_set_avail_min())
	if (HaveMy_snd_pcm_sw_params())
	if (HaveMy_snd_pcm_nonblock())
	if (HaveMy_snd_pcm_state())
	if (HaveMy_snd_pcm_prepare())
	if (HaveMy_snd_pcm_start())
	if (HaveMy_snd_pcm_resume())
	if (HaveMy_snd_pcm_avail_update())
	if (HaveMy_snd_pcm_writei())
	if (HaveMy_snd_pcm_drop())
	if (HaveMy_snd_strerror())
	{
		IsOk = trueblnr;
	}

	return IsOk;
}

#if 4 == kLn2SoundSampSz
#define MyDesiredFormat My_SND_PCM_FORMAT_S16
#else
#define MyDesiredFormat My_SND_PCM_FORMAT_U8
#endif

LOCALPROC MySound_Init0(void)
{
	My_snd_pcm_hw_params_t *hw_params = NULL;
	My_snd_pcm_sw_params_t *sw_params = NULL;
	unsigned int rrate = SOUND_SAMPLERATE;
	int err;

	buffer_size = desired_alsa_buffer_size;
	period_size = desired_alsa_period_size;

	/* Open the sound device */
	if (NULL == alsadev_name) {
		alsadev_name = getenv("AUDIODEV");
		if (NULL == alsadev_name) {
			alsadev_name = strdup("default");
		}
	}

	if ((err = My_snd_pcm_open(&pcm_handle, alsadev_name,
		My_SND_PCM_STREAM_PLAYBACK, My_SND_PCM_NONBLOCK)) < 0)
	{
		fprintf(stderr, "cannot open audio device %s (%s)\n",
			alsadev_name, My_snd_strerror(err));
		pcm_handle = NULL;
	} else
	/* Set some hardware parameters */
	if ((err = My_snd_pcm_hw_params_malloc(&hw_params)) < 0) {
		fprintf(stderr,
			"cannot allocate hardware parameter structure (%s)\n",
			My_snd_strerror(err));
		hw_params = NULL;
	} else
	if ((err = My_snd_pcm_hw_params_any(pcm_handle, hw_params)) < 0) {
		fprintf(stderr,
			"cannot initialize hardware parameter structure (%s)\n",
			My_snd_strerror(err));
	} else
	if ((err = My_snd_pcm_hw_params_set_access(pcm_handle,
		hw_params, My_SND_PCM_ACCESS_RW_INTERLEAVED)) < 0)
	{
		fprintf(stderr, "cannot set access type (%s)\n",
			My_snd_strerror(err));
	} else
	if ((err = My_snd_pcm_hw_params_set_format(pcm_handle,
		hw_params, MyDesiredFormat)) < 0)
	{
		fprintf(stderr, "cannot set sample format (%s)\n",
			My_snd_strerror(err));
	} else
	if ((err = My_snd_pcm_hw_params_set_rate_near(pcm_handle,
		hw_params, &rrate, NULL)) < 0)
	{
		fprintf(stderr, "cannot set sample rate (%s)\n",
			My_snd_strerror(err));
	} else
	if ((err = My_snd_pcm_hw_params_set_channels(pcm_handle,
		hw_params, 1)) < 0)
	{
		fprintf(stderr, "cannot set channel count (%s)\n",
			My_snd_strerror(err));
	} else
	if ((err = My_snd_pcm_hw_params_set_buffer_size_near(pcm_handle,
		hw_params, &buffer_size)) < 0)
	{
		fprintf(stderr, "cannot set buffer size count (%s)\n",
			My_snd_strerror(err));
	} else
	if ((err = My_snd_pcm_hw_params_set_period_size_near(pcm_handle,
		hw_params, &period_size, NULL)) < 0)
	{
		fprintf(stderr, "cannot set period size count (%s)\n",
			My_snd_strerror(err));
	} else
	if ((err = My_snd_pcm_hw_params(pcm_handle, hw_params)) < 0) {
		fprintf(stderr, "cannot set parameters (%s)\n",
			My_snd_strerror(err));
	} else
	{
		if (rrate != SOUND_SAMPLERATE) {
			fprintf(stderr, "Warning: sample rate is off by %i Hz\n",
				SOUND_SAMPLERATE - rrate);
		}

#if 0
		if (buffer_size != desired_alsa_buffer_size) {
			fprintf(stderr,
				"Warning: buffer size is off,"
				" desired %li, actual %li\n",
				desired_alsa_buffer_size, buffer_size);
		}

		if (period_size != desired_alsa_period_size) {
			fprintf(stderr,
				"Warning: period size is off,"
				" desired %li, actual %li\n",
				desired_alsa_period_size, period_size);
		}
#endif

		My_snd_pcm_hw_params_free(hw_params);
		hw_params = NULL;

		/* Set some software parameters */
		if ((err = My_snd_pcm_sw_params_malloc(&sw_params)) < 0) {
			fprintf(stderr,
				"cannot allocate software parameter structure (%s)\n",
				My_snd_strerror(err));
			sw_params = NULL;
		} else
		if ((err = My_snd_pcm_sw_params_current(pcm_handle,
			sw_params)) < 0)
		{
			fprintf(stderr,
				"Unable to determine current"
				" sw_params for playback: %s\n",
				My_snd_strerror(err));
		} else
		if ((err = My_snd_pcm_sw_params_set_start_threshold(pcm_handle,
			sw_params, 0x7FFFFFFF /* buffer_size - period_size */)) < 0)
		{
			fprintf(stderr,
				"Unable to set start threshold mode for playback: %s\n",
				My_snd_strerror(err));
		} else
#if 0
		if ((err = My_snd_pcm_sw_params_set_avail_min(pcm_handle,
			sw_params, period_size)) < 0)
		{
			fprintf(stderr,
				"Unable to set avail min for playback: %s\n",
				My_snd_strerror(err));
		} else
#endif
		/*
			snd_pcm_sw_params_set_xfer_align deprecated, but
			call if available. According to one report, bad results
			in old version of alsa lib if not called.
		*/
		if (HaveMy_snd_pcm_sw_params_set_xfer_align()
			&& ((err = My_snd_pcm_sw_params_set_xfer_align(pcm_handle,
			sw_params, 1)) < 0))
		{
			fprintf(stderr,
				"Unable to set transfer align for playback: %s\n",
				My_snd_strerror(err));
		} else
		if ((err = My_snd_pcm_sw_params(pcm_handle, sw_params)) < 0) {
			fprintf(stderr,
				"Unable to set sw params for playback: %s\n",
				My_snd_strerror(err));
		} else
		{
			My_snd_pcm_sw_params_free(sw_params);
			sw_params = NULL;

			My_snd_pcm_nonblock(pcm_handle, 0);

			goto label_done; /* success */
		}
	}

	/* clean up after failure */

	if (sw_params != NULL) {
		My_snd_pcm_sw_params_free(sw_params);
	}
	if (hw_params != NULL) {
		My_snd_pcm_hw_params_free(hw_params);
	}
	if (pcm_handle != NULL) {
		My_snd_pcm_close(pcm_handle);
		pcm_handle = NULL;
	}

label_done:
	;
}

LOCALFUNC blnr MySound_Init(void)
{
	if (HaveAlsaRoutines()) {
		MySound_Init0();
	}

	return trueblnr; /* keep going, even if no sound */
}

LOCALPROC MySound_UnInit(void)
{
	if (NULL != pcm_handle) {
		if (HaveMy_snd_pcm_close()) {
			My_snd_pcm_close(pcm_handle);
		}
		pcm_handle = NULL;
	}
	MyCloseAlsaLib();
}

GLOBALOSGLUPROC MySound_EndWrite(ui4r actL)
{
	if (MySound_EndWrite0(actL)) {
		ConvertSoundBlockToNative(TheSoundBuffer
			+ ((TheFillOffset - kOneBuffLen) & kAllBuffMask));
		if (NULL != pcm_handle) {
			MySound_WriteOut();
		}
	}
}

LOCALPROC MySound_SecondNotify(void)
{
	if (NULL != pcm_handle) {
		MySound_SecondNotify0();
	}
}

#define UsingAlsa 1
