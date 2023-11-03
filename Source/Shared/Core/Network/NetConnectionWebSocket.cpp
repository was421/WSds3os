#pragma once
/*
 * Dark Souls 3 - Open Server
 * Copyright (C) 2021 Tim Leonard
 *
 * This program is free software; licensed under the MIT license.
 * You should have received a copy of the license along with this program.
 * If not, see <https://opensource.org/licenses/MIT>.
 */

#include "Shared/Core/Network/NetConnectionWebSocket.h"
#include "Shared/Core/Utils/Logging.h"
#include "Shared/Core/Utils/DebugObjects.h"
#include "Shared/Core/Crypto/Cipher.h"

#include "Shared/Core/Network/WebSocketProvider/WebSocketProvider.h"

#include <cstring>

template <typename T>
bool is_uninitialized(std::weak_ptr<T> const& weak) {
    using wt = std::weak_ptr<T>;
    return !weak.owner_before(wt{}) && !wt{}.owner_before(weak);
}

namespace {
    // Maximum backlog of data in a packet streams send queue. Sending
    // packets beyond this will result in disconnect.
    static inline constexpr size_t k_max_send_buffer_size = 256 * 1024;
};


NetConnectionWebSocket::NetConnectionWebSocket(const std::string& InName)
    : Name(InName)
{
}

NetConnectionWebSocket::NetConnectionWebSocket(websocketpp::connection_hdl hdl, const std::string& InName, const NetIPAddress& InAddress)
    : Handle(hdl)
    , Name(InName)
    , IPAddress(InAddress)
{
}

NetConnectionWebSocket::~NetConnectionWebSocket()
{
    Disconnect();
}

bool NetConnectionWebSocket::Listen(int Port)
{
    Log("Listening on port %d, opened on endpoint %s instead", Port, this->Name.c_str());
    wsp->AddNewEndpoint(this->Name);
    return true;
}

std::shared_ptr<NetConnection> NetConnectionWebSocket::Accept()
{
    NetIPAddress addr;
    auto handle = wsp->Accept(this->Name);
    if (!is_uninitialized(handle)) {
        auto con = wsp->GetServer()->get_con_from_hdl(handle);
        auto remote = con->get_remote_endpoint();
        size_t colonPos = remote.find(':');
        if (colonPos != std::string::npos) {
            NetIPAddress::ParseString(remote.substr(0, colonPos), addr);
        }
        return std::make_shared<NetConnectionWebSocket>(handle, remote, addr);
    }
    return nullptr;
}

NetIPAddress NetConnectionWebSocket::GetAddress()
{
    return IPAddress;
}

bool NetConnectionWebSocket::Connect(std::string Hostname, int Port, bool ForceLastIpEntry)
{
    Warning("WebSocket Connect is a TODO");
    return false;
}

bool NetConnectionWebSocket::Peek(std::vector<uint8_t>& Buffer, int Offset, int Count, int& BytesReceived)
{
    auto str = this->wsp->Peek(this->Handle);
    if (str.empty())
    {
        BytesReceived = 0;
        return true;
    }
    if (Count > str.size())
    {
        ErrorS(GetName().c_str(), "Unable to peek ws packet. Peek size is larger than datagram size.");
        return false;
    }
    memcpy(Buffer.data() + Offset, str.data(), Count);
    BytesReceived = Count;

    return true;
}

bool NetConnectionWebSocket::Receive(std::vector<uint8_t>& Buffer, int Offset, int Count, int& BytesReceived)
{
    auto str = this->wsp->Read(this->Handle);
    if (str.empty()) {
        BytesReceived = 0;
        return true;
    }
    if (Count > str.size()) {
        ErrorS(GetName().c_str(), "Unable to recieve ws packet. Peek size is larger than datagram size.");
        return false;
    }
    memcpy(Buffer.data() + Offset, str.data(), Count);
    BytesReceived = Count;
    return true;
}

bool NetConnectionWebSocket::Send(const std::vector<uint8_t>& Buffer, int Offset, int Count)
{

    std::vector<uint8_t> CipheredBuffer;
    CipheredBuffer.resize(Count);

    memcpy(CipheredBuffer.data(), Buffer.data() + Offset, Count);

    size_t InsertOffset = SendQueue.size();
    size_t NewQueueSize = SendQueue.size() + CipheredBuffer.size();

    if (NewQueueSize > k_max_send_buffer_size)
    {
        WarningS(GetName().c_str(), "Failed to send packet, send queue is saturated.");
        return false;
    }

    SendQueue.resize(NewQueueSize);

    memcpy(SendQueue.data() + InsertOffset, CipheredBuffer.data(), CipheredBuffer.size());

    return true;
}

bool NetConnectionWebSocket::Disconnect()
{
    return this->wsp->Close(this->Handle);
}

std::string NetConnectionWebSocket::GetName()
{
    return Name;
}

void NetConnectionWebSocket::Rename(const std::string& InName)
{
    Name = InName;
}

bool NetConnectionWebSocket::IsConnected()
{
    auto con = this->server->get_con_from_hdl(this->Handle);
    if (con) {
        if (con->get_state() != websocketpp::session::state::open) {
            return false;
        }
        return true;
    }
    return false;
}

bool NetConnectionWebSocket::Pump()
{
    // Send any data that we are able to.
    while (SendQueue.size() > 0)
    {
        size_t BytesSent = 0;
        LogS(GetName().c_str(), "SEND %i", SendQueue.size());
        if (!this->wsp->Send(this->Handle, SendQueue, BytesSent))
        {
            WarningS(GetName().c_str(), "Failed to send on connection.");
            return true;
        }

        if (BytesSent == 0)
        {
            break;
        }
        else
        {
            SendQueue.erase(SendQueue.begin(), SendQueue.begin() + BytesSent);
        }
    }

    return false;
}