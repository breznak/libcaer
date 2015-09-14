/*
 * ear.h
 *
 *  Created on: Jan 6, 2014
 *      Author: llongi
 */

#ifndef LIBCAER_EVENTS_EAR_H_
#define LIBCAER_EVENTS_EAR_H_

#include "common.h"

#define EAR_SHIFT 8
#define EAR_MASK 0x00000007
#define GANGLION_SHIFT 11
#define GANGLION_MASK 0x0000007F
#define FILTER_SHIFT 18
#define FILTER_MASK 0x0000003F
#define CHANNEL_SHIFT 24
#define CHANNEL_MASK 0x000000FF

struct caer_ear_event {
	uint32_t data; // First because of valid mark.
	uint32_t timestamp;
}__attribute__((__packed__));

typedef struct caer_ear_event *caerEarEvent;

struct caer_ear_event_packet {
	struct caer_event_packet_header packetHeader;
	struct caer_ear_event events[];
}__attribute__((__packed__));

typedef struct caer_ear_event_packet *caerEarEventPacket;

static inline caerEarEventPacket caerEarEventPacketAllocate(uint32_t eventCapacity, uint16_t eventSource) {
	uint32_t eventSize = sizeof(struct caer_ear_event);
	size_t eventPacketSize = sizeof(struct caer_ear_event_packet) + (eventCapacity * eventSize);

	// Zero out event memory (all events invalid).
	caerEarEventPacket packet = calloc(1, eventPacketSize);
	if (packet == NULL) {
#if !defined(LIBCAER_LOG_NONE)
		caerLog(LOG_CRITICAL, "Ear Event", "Failed to allocate %zu bytes of memory for Ear Event Packet of capacity %"
		PRIu32 " from source %" PRIu16 ". Error: %d.", eventPacketSize, eventCapacity, eventSource,
		errno);
#endif
		return (NULL);
	}

	// Fill in header fields.
	caerEventPacketHeaderSetEventType(&packet->packetHeader, EAR_EVENT);
	caerEventPacketHeaderSetEventSource(&packet->packetHeader, eventSource);
	caerEventPacketHeaderSetEventSize(&packet->packetHeader, eventSize);
	caerEventPacketHeaderSetEventTSOffset(&packet->packetHeader, offsetof(struct caer_ear_event, timestamp));
	caerEventPacketHeaderSetEventCapacity(&packet->packetHeader, eventCapacity);

	return (packet);
}

static inline caerEarEvent caerEarEventPacketGetEvent(caerEarEventPacket packet, uint32_t n) {
	// Check that we're not out of bounds.
	if (n >= caerEventPacketHeaderGetEventCapacity(&packet->packetHeader)) {
#if !defined(LIBCAER_LOG_NONE)
		caerLog(LOG_CRITICAL, "Ear Event",
			"Called caerEarEventPacketGetEvent() with invalid event offset %" PRIu32 ", while maximum allowed value is %" PRIu32 ".",
			n, caerEventPacketHeaderGetEventCapacity(&packet->packetHeader));
#endif
		return (NULL);
	}

	// Return a pointer to the specified event.
	return (packet->events + n);
}

static inline uint32_t caerEarEventGetTimestamp(caerEarEvent event) {
	return (le32toh(event->timestamp));
}

static inline uint64_t caerEarEventGetTimestamp64(caerEarEvent event, caerEarEventPacket packet) {
	return ((U64T(caerEventPacketHeaderGetEventTSOverflow(&packet->packetHeader)) << TS_OVERFLOW_SHIFT)
		| U64T(caerEarEventGetTimestamp(event)));
}

// Limit Timestamp to 31 bits for compatibility with languages that have no unsigned integer (Java).
static inline void caerEarEventSetTimestamp(caerEarEvent event, int32_t timestamp) {
	if (timestamp < 0) {
		// Negative means using the 31st bit!
#if !defined(LIBCAER_LOG_NONE)
		caerLog(LOG_CRITICAL, "Ear Event", "Called caerEarEventSetTimestamp() with negative value!");
#endif
		return;
	}

	event->timestamp = htole32(timestamp);
}

static inline bool caerEarEventIsValid(caerEarEvent event) {
	return ((le32toh(event->data) >> VALID_MARK_SHIFT) & VALID_MARK_MASK);
}

static inline void caerEarEventValidate(caerEarEvent event, caerEarEventPacket packet) {
	if (!caerEarEventIsValid(event)) {
		event->data |= htole32(U32T(1) << VALID_MARK_SHIFT);

		// Also increase number of events and valid events.
		// Only call this on (still) invalid events!
		caerEventPacketHeaderSetEventNumber(&packet->packetHeader,
			caerEventPacketHeaderGetEventNumber(&packet->packetHeader) + 1);
		caerEventPacketHeaderSetEventValid(&packet->packetHeader,
			caerEventPacketHeaderGetEventValid(&packet->packetHeader) + 1);
	}
	else {
#if !defined(LIBCAER_LOG_NONE)
		caerLog(LOG_CRITICAL, "Ear Event", "Called caerEarEventValidate() on already valid event.");
#endif
	}
}

static inline void caerEarEventInvalidate(caerEarEvent event, caerEarEventPacket packet) {
	if (caerEarEventIsValid(event)) {
		event->data &= htole32(~(U32T(1) << VALID_MARK_SHIFT));

		// Also decrease number of valid events. Number of total events doesn't change.
		// Only call this on valid events!
		caerEventPacketHeaderSetEventValid(&packet->packetHeader,
			caerEventPacketHeaderGetEventValid(&packet->packetHeader) - 1);
	}
	else {
#if !defined(LIBCAER_LOG_NONE)
		caerLog(LOG_CRITICAL, "Ear Event", "Called caerEarEventInvalidate() on already invalid event.");
#endif
	}
}

static inline uint8_t caerEarEventGetEar(caerEarEvent event) {
	return U8T((le32toh(event->data) >> EAR_SHIFT) & EAR_MASK);
}

static inline void caerEarEventSetEar(caerEarEvent event, uint8_t ear) {
	event->data |= htole32((U32T(ear) & EAR_MASK) << EAR_SHIFT);
}

static inline uint8_t caerEarEventGetGanglion(caerEarEvent event) {
	return U8T((le32toh(event->data) >> GANGLION_SHIFT) & GANGLION_MASK);
}

static inline void caerEarEventSetGanglion(caerEarEvent event, uint8_t ganglion) {
	event->data |= htole32((U32T(ganglion) & GANGLION_MASK) << GANGLION_SHIFT);
}

static inline uint8_t caerEarEventGetFilter(caerEarEvent event) {
	return U8T((le32toh(event->data) >> FILTER_SHIFT) & FILTER_MASK);
}

static inline void caerEarEventSetFilter(caerEarEvent event, uint8_t filter) {
	event->data |= htole32((U32T(filter) & FILTER_MASK) << FILTER_SHIFT);
}

static inline uint8_t caerEarEventGetChannel(caerEarEvent event) {
	return U8T((le32toh(event->data) >> CHANNEL_SHIFT) & CHANNEL_MASK);
}

static inline void caerEarEventSetChannel(caerEarEvent event, uint8_t channel) {
	event->data |= htole32((U32T(channel) & CHANNEL_MASK) << CHANNEL_SHIFT);
}

#endif /* LIBCAER_EVENTS_EAR_H_ */