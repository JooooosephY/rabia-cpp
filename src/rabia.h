#pragma once

#include <memory>
#include <queue>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "messages.h"
#include "timesource.h"

struct INode {
    virtual ~INode() = default;
    virtual void Send(TMessage message) = 0;
    virtual void Drain() = 0;
};

struct IRsm {
    virtual ~IRsm() = default;
    virtual TMessage Read(TCmdReq message, uint64_t index) = 0;
    virtual void Write(Command command, uint64_t index) = 0;
    virtual Command Prepare(TCmdReq message) = 0;
};

struct TDummyRsm : public IRsm {
    //TMessage Read (TCmdReq message, uint64_t index) override;
    //void Write(Command, uint64_t index) override;
    //Command Prepare(TCmdReq message) override;

    private:
    uint64_t lastApplied;
    std::unordered_map<uint64_t, TSCommand>Log;
};

using TNodeDict = std::unordered_map<uint32_t, std::shared_ptr<INode>>;

enum class EStage : uint16_t {
    P1_STAGE = 1,
    P2S_STAGE = 2,
    P2V_STAGE = 3,
    DECIDED = 4
};

class TRabia {
public:
    TRabia(std::shared_ptr<IRsm> rsm, int node, const TNodeDict &ndoes);
    //void Process(ITimeSource::Time now, TMessage msg, const std::shared_ptr<INode>& replyTo = {});
    void Run(TMessage &msg, const std::shared_ptr<INode> &replyTo);

    // utilities
    const uint32_t GetId() const {
        return Id;
    }

    int getQuorumSize() const {
        return QuorumSize;
    }

    int GetNpeers() const {
        return Npeers;
    }

    int getNservers() const {
        return Nservers;
    }

private:
    std::shared_ptr<IRsm> Rsm;
    uint32_t Id;
    TNodeDict Nodes;
    int QuorumSize;
    int Npeers;
    int Nservers;

    uint32_t Seed = 31337;

    uint64_t cmdSeq = 1;
    uint64_t slotIdx = 1;    // equiv to seq in lab4, next slot index

    std::unordered_map<uint64_t, uint64_t> Storage = {};
    std::unordered_map<int, uint32_t> pendingRequests = {}; // key type undecided
    std::priority_queue<TSCommand> proposeQueue = {};
    std::unordered_map<uint64_t, EStage> mvcStage = {};
    std::unordered_map<uint64_t, TSCommand> weakMvcMyProposal = {};
    std::unordered_map<uint64_t, std::deque<TSCommand>> weakMvcProposals = {};
    std::unordered_map<uint64_t, TSCommand> weakMvcChosenCommand = {};
    std::unordered_map<uint64_t, uint16_t> weakMvcRound = {};  // logidx -> round
    std::unordered_map<uint64_t, std::deque<TStateMsg>> queuedStateMessages = {};
    std::unordered_map<uint64_t, std::deque<RSTSCommand>> weakMvcStateMessages = {};
    std::unordered_map<uint64_t, TSCommand> weaMvcStateCommand = {};
    std::unordered_map<uint64_t, TSCommand> weakMvcMyVote = {};
    std::unordered_map<uint64_t, std::deque<TVote>> queuedVoteMessages = {};
    std::unordered_map<uint64_t, std::deque<RVTSCommand>> weakMvcVotes = {};
    std::unordered_map<uint64_t, std::deque<TSCommand>> weakMvcVoteCommands = {};

    void HandleClientCommand(uint64_t client, Command cmd);
    void Bcast(TMessage msg);
    void propose_next_command();
    void process_proposal(uint64_t idx, uint32_t node, TSCommand cmd);
    void process_valid_proposal(uint64_t idx, TSCommand cmd);
    void transition_to_phase2state(uint64_t idx, uint16_t round, const TSCommand& smsg);
    void update_round(uint64_t idx, uint16_t round);
    void send_state_message(uint64_t idx);
    void replay_enqueued_state_messages(uint64_t idx);
    void process_state_message(uint64_t idx, int node, const TStateMsg& smsg);
    void count_state_message(uint64_t idx, TStateMsg& msg);
    void update_state_cmd(uint64_t idx, const TStateMsg& msg);
    void transition_to_phase2vote(uint64_t idx, uint64_t round, EVoteType vote, const std::optional<TSCommand>& command);
    void replay_enqueued_vote_msgs(uint64_t idx);
    void process_vote_message(uint64_t idx, uint32_t node, TVote vote)
};
