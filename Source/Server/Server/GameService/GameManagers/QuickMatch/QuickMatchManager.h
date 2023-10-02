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
#include "Protobuf/Protobufs.h"
#include "Server/GameService/Utils/GameIds.h"

struct Frpg2ReliableUdpMessage;
class Server;
class GameService;

// Handles client requests for joining/leaving quick matches (undead matches)

class QuickMatchManager
    : public GameManager
{
public:    
    QuickMatchManager(Server* InServerInstance, GameService* InGameServiceInstance);

    virtual MessageHandleResult OnMessageReceived(GameClient* Client, const Frpg2ReliableUdpMessage& Message) override;

    virtual std::string GetName() override;

    virtual void Poll() override;

    virtual void OnLostPlayer(GameClient* Client) override;
    
    size_t GetLiveCount() { return Matches.size(); }

protected:
    MessageHandleResult Handle_RequestSearchQuickMatch(GameClient* Client, const Frpg2ReliableUdpMessage& Message);
    MessageHandleResult Handle_RequestUnregisterQuickMatch(GameClient* Client, const Frpg2ReliableUdpMessage& Message);
    MessageHandleResult Handle_RequestUpdateQuickMatch(GameClient* Client, const Frpg2ReliableUdpMessage& Message);
    MessageHandleResult Handle_RequestJoinQuickMatch(GameClient* Client, const Frpg2ReliableUdpMessage& Message);
    MessageHandleResult Handle_RequestAcceptQuickMatch(GameClient* Client, const Frpg2ReliableUdpMessage& Message);
    MessageHandleResult Handle_RequestRejectQuickMatch(GameClient* Client, const Frpg2ReliableUdpMessage& Message);
    MessageHandleResult Handle_RequestRegisterQuickMatch(GameClient* Client, const Frpg2ReliableUdpMessage& Message);
    MessageHandleResult Handle_RequestSendQuickMatchStart(GameClient* Client, const Frpg2ReliableUdpMessage& Message);
    MessageHandleResult Handle_RequestSendQuickMatchResult(GameClient* Client, const Frpg2ReliableUdpMessage& Message);

private:
    struct Match
    {
        uint32_t HostPlayerId;
        std::string HostPlayerSteamId;

        Frpg2RequestMessage::QuickMatchGameMode GameMode;
        Frpg2RequestMessage::MatchingParameter MatchingParams;

        uint32_t MapId;
        OnlineAreaId AreaId;

        bool HasStarted = false;
    };

private:
    bool CanMatchWith(GameClient* Client, const Frpg2RequestMessage::RequestSearchQuickMatch& Request, const std::shared_ptr<Match>& Match);

    std::shared_ptr<Match> GetMatchByHost(uint32_t HostPlayerId);

private:
    Server* ServerInstance;
    GameService* GameServiceInstance;

    std::vector<std::shared_ptr<Match>> Matches;

};