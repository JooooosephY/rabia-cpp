#include <chrono>
#include <climits>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <iostream>
#include <algorithm>

#include <string.h>
#include <assert.h>

#include "rabia.h"
#include "messages.h"
#include "timesource.h"

static int common_coin() {
    return rand() % 2;
}

TRabia::TRabia(std::shared_ptr<IRsm> rsm, int node, const TNodeDict &nodes)
{
    Rsm = rsm;
    Id = node;
    Npeers = nodes.size();
    Nservers = nodes.size() + 1;
    QuorumSize = ((Npeers + 2 + Npeers % 2) / 2);
}

void HandleClientCommand(uint64_t client, Command command)
{
    
}

void TRabia::Run(TMessage &msg, const std::shared_ptr<INode> &replyTo)
{
    EMessageType msgType = msg.MessageType;
    switch (msgType)
    {
        case EMessageType::CMD_REQ:
        { 
            const TCmdReq* cmdmsg = static_cast<const TCmdReq*> (&msg);
            uint64_t client = msg.Src;
            Command cmd = cmdmsg->command;
            TRabia::HandleClientCommand(client, std::move(cmd));
            break;
        }
        default:
            break;
    }
}

