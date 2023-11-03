#pragma once
/*
 * Dark Souls 3 - Open WebSocket Server
 * Copyright (C) 2023 Church Guard
 *
 * This program is free software; licensed under the MIT license.
 * You should have received a copy of the license along with this program.
 * If not, see <https://opensource.org/licenses/MIT>.
 */

#pragma once

#include "Shared/Core/Network/NetConnection.h"
#include "Shared/Core/Network/WebSocketProvider/WebSocketProvider.h"

#include <stdlib.h>
#include <future>

class NetConnectionWebSocket
    : public NetConnection
{
public:

public:
    NetConnectionWebSocket(websocketpp::connection_hdl hdl, const std::string& InName, const NetIPAddress& InAddress);
    NetConnectionWebSocket(const std::string& InName);
    virtual ~NetConnectionWebSocket();

    virtual bool Listen(int Port) override;

    virtual std::shared_ptr<NetConnection> Accept() override;

    virtual bool Pump() override;

    virtual bool Connect(std::string Hostname, int Port, bool ForceLastIpEntry) override;

    virtual bool Peek(std::vector<uint8_t>& Buffer, int Offset, int Count, int& BytesReceived) override;
    virtual bool Receive(std::vector<uint8_t>& Buffer, int Offset, int Count, int& BytesReceived) override;
    virtual bool Send(const std::vector<uint8_t>& Buffer, int Offset, int Count) override;

    virtual bool Disconnect() override;

    virtual bool IsConnected() override;

    virtual NetIPAddress GetAddress() override;

    virtual std::string GetName() override;
    virtual void Rename(const std::string& Name) override;

private:
    websocketpp::connection_hdl Handle;
    std::string Name;
    NetIPAddress IPAddress;

    std::vector<uint8_t> SendQueue;

    //
    WebSocketProvider* wsp = WebSocketProvider::GetInstance();
    websocketpp::server<websocketpp::config::asio>* server;
};