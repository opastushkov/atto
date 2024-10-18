#pragma once

struct Message
{
	uint16_t	messageSize;
	uint8_t		messageType;
	uint64_t	messageId;
	uint64_t	messageData;
};