/*
 * Dark Souls 3 - Open Server
 * Copyright (C) 2021 Tim Leonard
 *
 * This program is free software; licensed under the MIT license.
 * You should have received a copy of the license along with this program.
 * If not, see <https://opensource.org/licenses/MIT>.
 */

#pragma once

#include "Server/GameService/GameManager.h"
#include "Server/GameService/Utils/OnlineAreaPool.h"
#include "Server/Database/DatabaseTypes.h"

struct Frpg2ReliableUdpMessage;
class Server;

// Handles client requests relating to leaving
// and replaying ghosts in the world. These are the ones that
// appear when you are near things like bonfires.

class GhostManager
    : public GameManager
{
public:    
    GhostManager(Server* InServerInstance);

    virtual bool Init() override;
    virtual void TrimDatabase() override;

    virtual MessageHandleResult OnMessageReceived(GameClient* Client, const Frpg2ReliableUdpMessage& Message) override;

    virtual std::string GetName() override;

    size_t GetLiveCount() { return LiveCache.GetTotalEntries(); }

protected:
    MessageHandleResult Handle_RequestCreateGhostData(GameClient* Client, const Frpg2ReliableUdpMessage& Message);
    MessageHandleResult Handle_RequestGetGhostDataList(GameClient* Client, const Frpg2ReliableUdpMessage& Message);

private:
    Server* ServerInstance;

    OnlineAreaPool<Ghost> LiveCache;

};