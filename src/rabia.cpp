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

void TRabia::Bcast(TMessage msg) 
{
    for (auto it = Nodes.begin(); it != Nodes.end(); it++)
    {
        uint32_t dst = it->first;
        //auto Node = it->second;
        msg.Dst = dst;
        it->second->Send(msg);
    }
    
}

void TRabia::HandleClientCommand(uint64_t client, Command cmd)
{
    TSCommand tscmd{
        .idx = cmdSeq,
        .node_id = Id,
        .command = cmd
    };
    TReplicate repMsg{
        .tsCommand = tscmd,
    };
    repMsg.Type = (uint32_t)repMsg.MessageType;
    Bcast(repMsg);
    printf("%u received client command\n", Id);
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

