/*
 * Dark Souls 3 - Open Server
 * Copyright (C) 2021 Tim Leonard
 *
 * This program is free software; licensed under the MIT license.
 * You should have received a copy of the license along with this program.
 * If not, see <https://opensource.org/licenses/MIT>.
 */

#pragma once

#include "Shared/Core/Utils/Endian.h"

#include <vector>

 // All the id's of message type we can receive.
enum class Frpg2MessageType
{
    Reply = 0x0,

    // Authentication flow messages.
    KeyMaterial = 1,
    SteamTicket = 3,
    GetServiceStatus = 2,
    RequestQueryLoginServerInfo = 5,
    RequestHandshake = 6,
};

// See ds3server_packet.bt for commentry on what each of these
// fields appears to represent.
#pragma pack(push,1)
struct Frpg2MessageHeader
{
public:

    uint32_t header_size        = 12; // As far as I can tell this is always 12.
    Frpg2MessageType msg_type   = Frpg2MessageType::Reply;     
    uint32_t msg_index          = 0x00; // Request/response messages have the same value for this.

    void SwapEndian()
    {
        header_size = HostOrderToBigEndian(header_size);
        msg_type = HostOrderToBigEndian(msg_type);
        msg_index = HostOrderToLittleEndian(msg_index);
    }
};

struct Frpg2MessageResponseHeader
{
public:

    uint32_t unknown_1 = 0x0;
    uint32_t unknown_2 = 0x1;
    uint32_t unknown_3 = 0x0;
    uint32_t unknown_4 = 0x0;

    void SwapEndian()
    {
        unknown_1 = HostOrderToBigEndian(unknown_1);
        unknown_2 = HostOrderToBigEndian(unknown_2);
        unknown_3 = HostOrderToBigEndian(unknown_3);
        unknown_4 = HostOrderToBigEndian(unknown_4);
    }
};
#pragma pack(pop)

static_assert(sizeof(Frpg2MessageHeader) == 12, "Message header is not expected size.");

struct Frpg2Message
{
public:

    Frpg2MessageHeader Header;
    Frpg2MessageResponseHeader ResponseHeader; // Only exists if its a response :thinking:

    // Length of this payload should be the same
    // as the value stored in Header.payload_length
    std::vector<uint8_t> Payload;

    std::string Disassembly;

};