/*
 * Dark Souls 3 - Open Server
 * Copyright (C) 2021 Tim Leonard
 *
 * This program is free software; licensed under the MIT license.
 * You should have received a copy of the license along with this program.
 * If not, see <https://opensource.org/licenses/MIT>.
 */

#include "Server/GameService/GameManagers/Visitor/VisitorManager.h"
#include "Server/GameService/GameClient.h"
#include "Server/GameService/GameService.h"
#include "Server/Streams/Frpg2ReliableUdpMessage.h"
#include "Server/Streams/Frpg2ReliableUdpMessageStream.h"

#include "Config/RuntimeConfig.h"
#include "Server/Server.h"

#include "Config/BuildConfig.h"
#include "Server/GameService/Utils/NRSSRSanitizer.h"

#include "Shared/Core/Utils/Logging.h"
#include "Shared/Core/Utils/Strings.h"

VisitorManager::VisitorManager(Server* InServerInstance, GameService* InGameServiceInstance)
    : ServerInstance(InServerInstance)
    , GameServiceInstance(InGameServiceInstance)
{
}

MessageHandleResult VisitorManager::OnMessageReceived(GameClient* Client, const Frpg2ReliableUdpMessage& Message)
{
    if (Message.Header.msg_type == Frpg2ReliableUdpMessageType::RequestGetVisitorList)
    {
        return Handle_RequestGetVisitorList(Client, Message);
    }
    else if (Message.Header.msg_type == Frpg2ReliableUdpMessageType::RequestVisit)
    {
        return Handle_RequestVisit(Client, Message);
    }
    else if (Message.Header.msg_type == Frpg2ReliableUdpMessageType::RequestRejectVisit)
    {
        return Handle_RequestRejectVisit(Client, Message);
    }

    return MessageHandleResult::Unhandled;
}

bool VisitorManager::CanMatchWith(const Frpg2RequestMessage::MatchingParameter& Request, const std::shared_ptr<GameClient>& Match)
{
    const RuntimeConfig& Config = ServerInstance->GetConfig();
    bool IsInvasion = (Match->GetPlayerState().GetVisitorPool() != Frpg2RequestMessage::VisitorPool::VisitorPool_Way_of_Blue);

    const RuntimeConfigMatchingParameters* MatchingParams = &Config.CovenantInvasionMatchingParameters;
    if (!IsInvasion)
    {
        MatchingParams = &Config.WayOfBlueMatchingParameters;
    }

    // Matching globally disabled?
    bool IsDisabled = IsInvasion ? Config.DisableInvasionAutoSummon : Config.DisableCoopAutoSummon;
    if (IsDisabled)
    {
        return false;
    }

    // Check matching parameters.
    if (!MatchingParams->CheckMatch(
            Request.soul_level(), Request.weapon_level(),
            Match->GetPlayerState().GetSoulLevel(), Match->GetPlayerState().GetMaxWeaponLevel(),
            Request.password().size() > 0
        ))
    {
        return false;
    }

    return true;
}

MessageHandleResult VisitorManager::Handle_RequestGetVisitorList(GameClient* Client, const Frpg2ReliableUdpMessage& Message)
{
    Frpg2RequestMessage::RequestGetVisitorList* Request = (Frpg2RequestMessage::RequestGetVisitorList*)Message.Protobuf.get();
    
    std::vector<std::shared_ptr<GameClient>> PotentialTargets = GameServiceInstance->FindClients([this, Client, Request](const std::shared_ptr<GameClient>& OtherClient) {
        if (Client == OtherClient.get())
        {
            return false;
        }
        if (Request->visitor_pool() != OtherClient->GetPlayerState().GetVisitorPool())
        {
            return false;
        }
        return CanMatchWith(Request->matching_parameter(), OtherClient); 
    });

    // TODO: Sort potential targets based on prioritization (more summons etc)

    Frpg2RequestMessage::RequestGetVisitorListResponse Response;
    Response.set_map_id(Request->map_id());
    Response.set_online_area_id(Request->online_area_id());

    int CountToSend = std::min((int)Request->max_visitors(), (int)PotentialTargets.size());
    for (int i = 0; i < CountToSend; i++)
    {
        std::shared_ptr<GameClient> OtherClient = PotentialTargets[i];

        Frpg2RequestMessage::VisitorData* Data = Response.add_visitors();
        Data->set_player_id(OtherClient->GetPlayerState().GetPlayerId());
        Data->set_player_steam_id(OtherClient->GetPlayerState().GetSteamId());
    }

    if (!Client->MessageStream->Send(&Response, &Message))
    {
        WarningS(Client->GetName().c_str(), "Disconnecting client as failed to send RequestGetVisitorListResponse response.");
        return MessageHandleResult::Error;
    }

    return MessageHandleResult::Handled;
}

MessageHandleResult VisitorManager::Handle_RequestVisit(GameClient* Client, const Frpg2ReliableUdpMessage& Message)
{
    ServerDatabase& Database = ServerInstance->GetDatabase();
    PlayerState& Player = Client->GetPlayerState();

    Frpg2RequestMessage::RequestVisit* Request = (Frpg2RequestMessage::RequestVisit*)Message.Protobuf.get();

    bool bSuccess = true;

    // Check client still exists.
    std::shared_ptr<GameClient> TargetClient = GameServiceInstance->FindClientByPlayerId(Request->player_id());
    if (!TargetClient)
    {
        WarningS(Client->GetName().c_str(), "Client attempted to target unknown (or disconnected) client for visit %i.", Request->player_id());
        bSuccess = false;
    }

    // If success sent push to target client.
    if (bSuccess && TargetClient)
    {
        Frpg2RequestMessage::PushRequestVisit PushMessage;
        PushMessage.set_push_message_id(Frpg2RequestMessage::PushID_PushRequestVisit);
        PushMessage.set_player_id(Player.GetPlayerId());
        PushMessage.set_player_steam_id(Player.GetSteamId());
        PushMessage.set_data(Request->data().c_str(), Request->data().size());
        PushMessage.set_visitor_pool(Request->visitor_pool());
        PushMessage.set_map_id(Request->map_id());
        PushMessage.set_online_area_id(Request->online_area_id());

#ifdef _DEBUG
        LogS(Client->GetName().c_str(), "Sending push requesting visit to player: %s", TargetClient->GetName().c_str());
        Log("push_message_id: %i", PushMessage.push_message_id());
        Log("player_id: %i", PushMessage.player_id());
        Log("player_steam_id: %s", PushMessage.player_steam_id().c_str());
        Log("data: %i bytes", PushMessage.data().size());
        Log("visitor_pool: %i", PushMessage.visitor_pool());
        Log("map_id: %i", PushMessage.map_id());
        Log("online_area_id: %i", PushMessage.online_area_id());
#endif

        if (!TargetClient->MessageStream->Send(&PushMessage))
        {
            WarningS(TargetClient->GetName().c_str(), "Failed to send PushRequestBreakInTarget to target of visit.");
            bSuccess = false;
        }
    }

    // Empty response, not sure what purpose this serves really other than saying message-recieved. Client
    // doesn't work without it though.
    Frpg2RequestMessage::RequestVisitResponse Response;
    if (!Client->MessageStream->Send(&Response, &Message))
    {
        WarningS(Client->GetName().c_str(), "Disconnecting client as failed to send RequestVisitResponse response.");
        return MessageHandleResult::Error;
    }

    // Otherwise send rejection to client.
    if (!bSuccess)
    {
#ifdef _DEBUG
        LogS(Client->GetName().c_str(), "Sending push rejecting visit of player, as could not be found.");
#endif

        Frpg2RequestMessage::PushRequestRejectVisit PushMessage;
        PushMessage.set_push_message_id(Frpg2RequestMessage::PushID_PushRequestRejectVisit);
        PushMessage.set_player_id(Request->player_id());
        PushMessage.set_visitor_pool(Request->visitor_pool());   
        PushMessage.set_steam_id("");
        PushMessage.set_unknown_5(0);
        if (TargetClient)
        {
            PushMessage.set_steam_id(TargetClient->GetPlayerState().GetSteamId());
        }

        if (!Client->MessageStream->Send(&PushMessage))
        {
            WarningS(Client->GetName().c_str(), "Failed to send PushRequestRejectVisit.");
            return MessageHandleResult::Error;
        }
    }
    // On success the server immediately sends a PushRequestRemoveVisitor message.
    else
    {
        Frpg2RequestMessage::PushRequestRemoveVisitor PushMessage;
        PushMessage.set_push_message_id(Frpg2RequestMessage::PushID_PushRequestRemoveVisitor);
        PushMessage.set_player_id(Request->player_id());
        if (TargetClient)
        {
            PushMessage.set_player_steam_id(TargetClient->GetPlayerState().GetSteamId());
        }
        PushMessage.set_visitor_pool(Request->visitor_pool());

        if (!Client->MessageStream->Send(&PushMessage, &Message))
        {
            WarningS(Client->GetName().c_str(), "Disconnecting client as failed to send PushRequestRemoveVisitor response.");
            return MessageHandleResult::Error;
        }

        std::string PoolStatisticKey = StringFormat("Visitor/TotalVisitsRequested/Pool=%u", (uint32_t)Request->visitor_pool());
        Database.AddGlobalStatistic(PoolStatisticKey, 1);
        Database.AddPlayerStatistic(PoolStatisticKey, Player.GetPlayerId(), 1);

        std::string TypeStatisticKey = StringFormat("Visitor/TotalVisitsRequested");
        Database.AddGlobalStatistic(TypeStatisticKey, 1);
        Database.AddPlayerStatistic(TypeStatisticKey, Player.GetPlayerId(), 1);
    }

    return MessageHandleResult::Handled;
}

MessageHandleResult VisitorManager::Handle_RequestRejectVisit(GameClient* Client, const Frpg2ReliableUdpMessage& Message)
{
    PlayerState& Player = Client->GetPlayerState();

    Frpg2RequestMessage::RequestRejectVisit* Request = (Frpg2RequestMessage::RequestRejectVisit*)Message.Protobuf.get();

    // Get client who initiated the visit.
    std::shared_ptr<GameClient> InitiatorClient = GameServiceInstance->FindClientByPlayerId(Request->player_id());
    if (!InitiatorClient)
    {
        WarningS(Client->GetName().c_str(), "Client rejected visit from unknown (or disconnected) client %i.", Request->player_id());
        return MessageHandleResult::Handled;
    }

#ifdef _DEBUG
    LogS(Client->GetName().c_str(), "Received rejection of visit requested by player: %s", InitiatorClient->GetName().c_str());
    Log("player_id: %i", Request->player_id());
    Log("map_id: %i", Request->map_id());
    Log("online_area_id: %i", Request->online_area_id());
    Log("visitor_pool: %i", Request->visitor_pool());
    Log("unknown_5: %i", Request->unknown_5());
#endif

    // Reject the visit
    Frpg2RequestMessage::PushRequestRejectVisit PushMessage;
    PushMessage.set_push_message_id(Frpg2RequestMessage::PushID_PushRequestRejectVisit);
    PushMessage.set_player_id(Player.GetPlayerId());
    if (Request->has_visitor_pool())
    {
        PushMessage.set_visitor_pool(Request->visitor_pool());
    }
    PushMessage.set_steam_id(Player.GetSteamId());
    PushMessage.set_unknown_5(0);

    if (!InitiatorClient->MessageStream->Send(&PushMessage))
    {
        WarningS(Client->GetName().c_str(), "Failed to send PushRequestRejectBreakInTarget to invader client %s.", InitiatorClient->GetName().c_str());
    }

    // Empty response, not sure what purpose this serves really other than saying message-recieved. Client
    // doesn't work without it though.
    Frpg2RequestMessage::RequestRejectVisitResponse Response;
    if (!Client->MessageStream->Send(&Response, &Message))
    {
        WarningS(Client->GetName().c_str(), "Disconnecting client as failed to send RequestRejectVisitResponse response.");
        return MessageHandleResult::Error;
    }

    return MessageHandleResult::Handled;
}

std::string VisitorManager::GetName()
{
    return "Visitor";
}
