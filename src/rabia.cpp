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

void check_assumption(bool pred, const std::string& explanation) {
    if (!pred) {
        throw std::runtime_error("[" + whoami() + "] " + explanation);
    }
}

// Look at the priority queue to pick the next item the node should propose.
void TRabia::propose_next_command(){
    auto& q = proposeQueue;
    auto& s = slotIdx;
    auto& props = weakMvcMyProposal;

    TSCommand c = q.top();
    q.pop();
    TMessage proposal = TProposal(cmdSeq, c);
    Bcast(proposal);
    printf("%u proposing %s for %lu\n", Id, c.command.c_str(), s);

    // Update state
    cmdSeq = s + 1;
    props[s] = c;
}

// 有问题
// Phase 1: Proposal Called by `run` to process proposal messages as a part of Phase 1 of Weak MVC.
void TRabia::process_proposal(uint64_t idx, uint32_t node, TSCommand cmd){
    auto it = mvcStage.find(idx);
    if (it->second == EStage::P1_STAGE) {
        process_valid_proposal(idx, cmd);
    } else {
        if (it->second == EStage::DECIDED) {
            printf("%u received proposal from %u about decided index %lu\n", Id, node, idx);
            TDecided decidedMsg{
                .log_idx = idx,
                .command = cmd_log[idx]
            };
            Nodes[node]->Send(decidedMsg);
        } else {
            printf("%u Received proposal from %u for %lu, but have already transitioned to Ben-Or\n", Id, node, idx);
        }
    }
}

// 有问题
// using TSCommandPair = std::pair<uint64_t, uint64_t>; // { timestamp, count }
void TRabia::process_valid_proposal(uint64_t idx, TSCommand cmd) {
    // Extracting relevant fields from state
    auto& p = state.weak_mvc_proposals;
    auto& q = state.quorum_size;
    auto& scmd = state.weak_mvc_state_cmd;

    // Get the proposals for the given index
    auto& proposals = p[idx];

    // Find the count for the given timestamp
    uint32_t count = 0;
    for (auto& entry : proposals) {
        if (entry.first == cmd.idx) {
            entry.second++;
            count = entry.second;
            break;
        }
    }

    // If the timestamp is not found, add it to the proposals
    if (count == 0) {
        proposals.emplace_back(cmd.idx, 1);
        count = 1;
    }

    // Calculate the total number of proposals
    uint32_t n_proposals = 0;
    for (const auto& entry : proposals) {
        n_proposals += entry.second;
    }

    // Update state with the latest proposals
    p[idx] = proposals;

    // Handle based on proposal count and quorum size
    if (count >= q) {
        // Transition to Ben-Or with state cmd
        std::cout << whoami() << ", " << idx << " going to Ben-Or with state " << cmd << std::endl;
        scmd[idx] = cmd;
        transition_to_phase2state(idx, 1, cmd);
    } else if (n_proposals >= q) {
        // Transition to Ben-Or with state bot
        std::cout << whoami() << ", " << idx << " going to Ben-Or with state bot" << std::endl;
        transition_to_phase2state(idx, 1, EStateType::BOT);
    }
}

void TRabia::transition_to_phase2state(uint64_t idx, uint16_t round, const TSCommand& smsg) {
    weakMvcChosenCommand[idx] = smsg;
    update_round(idx, round);

    send_state_message(idx);

    // 3. 重放已排队的状态消息
    replay_enqueued_state_messages(idx);
}

void TRabia::update_round(uint64_t idx, uint16_t round){
    weakMvcRound[idx] = round;
    mvcStage[idx] = EStage::P2S_STAGE; 
    weakMvcStateMessages.erase(idx);
    queuedStateMessages.erase(idx);
    weakMvcVotes.erase(idx);
}

//有问题 cmd里的state问题
void TRabia::send_state_message(uint64_t idx){
    auto commands = weakMvcChosenCommand;
    auto rounds = weakMvcRound;

    // 2. 根据命令类型创建相应的状态消息
    TStateMsg sm;
    auto cmd = commands[idx];
    auto round = rounds[idx];
    if (cmd == EStateType::BOT) {
        sm = TStateMsg(idx, round, cmd);
    } else {
        sm = TStateMsg(idx, round, cmd);
    }

    // 3. 使用广播函数将状态消息发送给所有节点
    Bcast(sm);
}

void TRabia::replay_enqueued_state_messages(uint64_t idx) {
    auto msg_q = queuedStateMessages;

    auto& queue = msg_q[idx];
    std::reverse(queue.begin(), queue.end());

    auto node = Nodes[Id];
    
    for (const auto& msg : queue) {
        msg.Dst = Id;
        node->Send(msg);
    }

    msg_q.erase(idx);

}

// 有问题 cmd_log
void TRabia::process_state_message(uint64_t idx, uint32_t node, const TStateMsg& smsg){
    auto& stage = mvcStage;
    auto& msg_q = queuedStateMessages;
    auto& rounds = weakMvcRound;

    uint16_t r = smsg.rstsComand.round;

    if (stage[idx] == EStage::P2S_STAGE && rounds[idx] == r) {
        count_state_message(idx, node, smsg);
    } else {
        if (stage[idx] == EMessageType::DECIDED) {
            printf(whoami() + " received state message from " + node->getName() +
                            " about decided index " + std::to_string(idx));
            Nodes[node]->Send(TDecided(idx, cmd_log[idx]));
        } else {
            if (rounds[idx] <= r) {
                // Update queued state messages
                msg_q[idx].push_back(smsg);
            } else {
                printf(whoami() + " dropping state message from old round (" +
                                std::to_string(r) + ", " + std::to_string(rounds[idx]) + ")");
            }
        }
    }
}

void TRabia::count_state_message(uint64_t idx, TStateMsg& msg){
    auto& s = weakMvcStateMessages;
    auto& q = QuorumSize;

    uint16_t r = msg.rstsComand.round;
    TSCommand cmd = msg.rstsComand.tsCommand;
    EStateType t = msg.rstsComand.state;

    auto& smsgs = s[idx];
    auto it = smsgs.find(t);
    int count = 0;
    if (it != smsgs.end()) {
        count = it->second;
    }

    int n_smsgs = 0;
    for (const auto& kv : smsgs) {
        n_smsgs += kv.second;
    }
    
    update_state_cmd(idx, msg);

    if (count >= q) {
        printf(whoami() + "," + std::to_string(idx) + " Round " + std::to_string(r) +
                       " vote for command " + cmd.toString());
        transition_to_phase2vote(idx, r, EVoteType::CMD_VOTE, cmd);
    } else {
        
        if (n_smsgs >= q) {
            printf(whoami() + "," + std::to_string(idx) + " Round " + std::to_string(r) +
                           " vote for ?");
            transition_to_phase2vote(idx, r, EVoteType::QMARK_VOTE, cmd);
        }
    }
}

void TRabia::update_state_cmd(uint64_t idx, const TStateMsg& msg){
    auto& scmds = weaMvcStateCommand;
    auto t = msg.rstsComand.state;
    auto cmd = msg.rstsComand.tsCommand;

    printf(whoami() + "," + std::to_string(idx) + " update state command " +
                   cmd.toString() + " (" + std::to_string(t) + ")");

    if (t == EStateType::CMD) {
        auto it = scmds.find(idx);
        if (it != scmds.end()) {
            check_assumption(it->second == cmd, "More than 1 non-bot command in the mix.");
        }
        scmds[idx] = cmd;
    }
}

void TRabia::transition_to_phase2vote(uint64_t idx, uint64_t round, EVoteType vote, const std::optional<TSCommand>& command){
    auto msg = TVote(idx, RVTSCommand(round, vote, command));
    Bcast(msg);

    if (vote == EVoteType::CMD_VOTE) {
        weakMvcVoteCommands[idx] = command;
    }

    mvcStage[idx] = EStage::P2V_STAGE;
    weakMvcMyVote[idx] = vote; //有问题，这个map应该为idx, EVoteType

    replay_enqueued_vote_msgs(idx);
}

void TRabia::replay_enqueued_vote_msgs(uint64_t idx) {
    auto& vmsgq = queuedVoteMessages;
    auto& queue = vmsgq[idx];
    std::reverse(queue.begin(), queue.end());

    auto node = Nodes[Id];
    
    for (const auto& msg : queue) {
        msg.Dst = Id;
        node->Send(msg);
    }
    
    vmsgq.erase(idx);
}

void TRabia::process_vote_message(uint64_t idx, uint32_t node, TVote vote) {
    auto& stage = mvcStage;
    auto& rounds = weakMvcRound;
    auto& vote_q = queuedVoteMessages;

    if (stage[idx] == EStage::P2V_STAGE && rounds[idx] == vote.rvtsCommand.round) {
        count_votes(idx, node, vote);
    } else {
        if (stage[idx] == EStage::DECIDED) {
            // Send decided message
            TDecided msg = TDecided(idx, cmdLog[idx]);
            msg.Dst = node;
            Nodes[Id]->Send(mag);
        } else {
            if (rounds[idx] <= vote.rvtsCommand.round) {
                // Queue the vote message
                vote_q[idx].push_back(vote);
            } else {
                printf(whoami() + ", " + std::to_string(idx) +
                                " dropping vote message from old round (" +
                                std::to_string(vote.rvtsCommand.round) + ", " +
                                std::to_string(rounds[idx]) + ")");
            }
        }
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

