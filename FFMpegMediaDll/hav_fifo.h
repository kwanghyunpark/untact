
#pragma once

#ifdef __cplusplus
#define __STDC_CONSTANT_MACROS
#ifdef _STDINT_H
#undef _STDINT_H
#endif
# include "stdint.h"

extern "C"{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavdevice/avdevice.h"
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/samplefmt.h>
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libavutil/common.h>
#include <libavutil/time.h>
};

#endif

#include <stdio.h>
#include <SDL.h>
#include <SDL_thread.h>
#include <windows.h>


namespace hav{

	// packet list for audio, video, and haptic data
	typedef struct PacketList 
	{ 
		int64_t time_stamp;
		AVPacket pkt;
		struct PacketList *next;
	} PacketList;


	typedef struct PacketQueue 
	{
		int64_t time_stamp;
		PacketList *first_pkt, *last_pkt;
		int nb_packets;
		int size;
		int abort_request;
		SDL_mutex *mutex;
		SDL_cond *cond;
	} PacketQueue;


	static int packet_queue_put(PacketQueue *q, AVPacket *pkt, int64_t time_stamp);

	static int packet_queue_put_private(PacketQueue *q, AVPacket *pkt, int64_t time_stamp)
	{
		PacketList *pkt1;

		if (q->abort_request)
			return -1;

		pkt1 = (PacketList*)av_malloc(sizeof(PacketList));
		if (!pkt1)
			return -1;
		pkt1->pkt = *pkt;
		pkt1->next = NULL;
		pkt1->time_stamp = time_stamp; // timestamp unit: us

		if (!q->last_pkt)
			q->first_pkt = pkt1;
		else
			q->last_pkt->next = pkt1;
		q->last_pkt = pkt1;
		q->nb_packets++;
		q->size += pkt1->pkt.size + sizeof(*pkt1);
		// XXX: should duplicate packet data in DV case 
		SDL_CondSignal(q->cond);
		return 0;
	}

	static int packet_queue_put(PacketQueue *q, AVPacket *pkt, int64_t time_stamp)
	{
		int ret;

		// duplicate the packet 
		if (av_dup_packet(pkt) < 0)
			return -1;

		SDL_LockMutex(q->mutex);
		ret = packet_queue_put_private(q, pkt, time_stamp);
		SDL_UnlockMutex(q->mutex);

		if (ret < 0)
			av_free_packet(pkt);

		return ret;
	}

	// packet queue handling 
	static void packet_queue_init(PacketQueue *q)
	{
		memset(q, 0, sizeof(PacketQueue));
		q->mutex = SDL_CreateMutex();
		q->cond = SDL_CreateCond();
		q->abort_request = 1;
		q->time_stamp = av_gettime();
	}

	static void packet_queue_flush(PacketQueue *q)
	{
		PacketList *pkt, *pkt1;

		SDL_LockMutex(q->mutex);
		for (pkt = q->first_pkt; pkt != NULL; pkt = pkt1) {
			pkt1 = pkt->next;
			av_free_packet(&pkt->pkt);
			av_freep(&pkt);
		}
		q->last_pkt = NULL;
		q->first_pkt = NULL;
		q->nb_packets = 0;
		q->size = 0;
		q->time_stamp = -1;
		SDL_UnlockMutex(q->mutex);
	}

	static void packet_queue_destroy(PacketQueue *q)
	{
		packet_queue_flush(q);
		SDL_DestroyMutex(q->mutex);
		SDL_DestroyCond(q->cond);
	}

	static void packet_queue_abort(PacketQueue *q)
	{
		SDL_LockMutex(q->mutex);
		q->abort_request = 1;
		SDL_CondSignal(q->cond);
		SDL_UnlockMutex(q->mutex);
	}

	static void packet_queue_start(PacketQueue *q)
	{
		SDL_LockMutex(q->mutex);
		q->abort_request = 0;
		SDL_UnlockMutex(q->mutex);
	}

	// return < 0 if aborted, 0 if no packet and > 0 if packet.  
	static int packet_queue_get(PacketQueue *q, AVPacket *pkt, int block, int64_t* timestamp)
	{
		PacketList *pkt1;
		int ret;

		SDL_LockMutex(q->mutex);

		for (;;) {
			if (q->abort_request) {
				ret = -1;
				break;
			}

			pkt1 = q->first_pkt;
			if (pkt1) {
				q->first_pkt = pkt1->next;
				if (!q->first_pkt)
					q->last_pkt = NULL;
				q->nb_packets--;
				q->size -= pkt1->pkt.size + sizeof(*pkt1);
				*pkt = pkt1->pkt;
				*timestamp = pkt1->time_stamp;
				q->time_stamp = pkt1->time_stamp;

				av_free(pkt1);
				ret = 1;
				break;
			} else if (!block) {
				ret = 0;
				break;
			} else {
				SDL_CondWait(q->cond, q->mutex);
			}
		}
		SDL_UnlockMutex(q->mutex);
		return ret;
	}

}
