#pragma once
#include <chrono>
#include <memory>
#include <vector>

#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <typeinfo>

enum class Operation : uint32_t {
    SET = 0,
    GET = 1,
    DEL = 2
    //LIST = 3
};

enum class EMessageType : uint32_t {
    PROTOCAL = 0,
    LOG_ENTRY = 1,
    REPLICATE = 2,
    PROPOSAL = 3,
    STATE = 4,
    VOTE = 5,
    DECIDED = 6,
    RESPONSE = 7
};  // equiv to _valid_types in lab4, #1 is from client

// used in state messages
enum class EStateType : uint16_t {
    CMD = 0,
    BOT = 1
};

// used in vote messages
enum class EVoteType : uint16_t {
    CMD_VOTE = 0,
    QMARK_VOTE = 1
};


struct Command {
    uint64_t client_seq;
    Operation operation;
    uint64_t key;
    uint64_t value;
};

struct SetCommand : public Command{
    static const Operation operation = Operation::SET;
};

struct GetCommand : public Command {
    static const Operation operation = Operation::GET;
    static const uint64_t value = 0;
};

struct DelCommand : public Command {
    static const Operation operation = Operation::DEL;
    static const uint64_t value = 0;
};

// struct ListCommand : public Command {
//     static const Operation operation = Operation::LIST;
//     static const uint64_t key = 0;
//     static const uint64_t value = 0;
// };

static_assert(sizeof(DelCommand) == 32);

struct RSCommand {
    uint16_t round;
    EStateType state;
    Command command;
};  // name stands for: round state command

struct RVCommand {
    uint16_t round;
    EVoteType vote;
    Command command;
};  // name stands for: round vote command


static_assert(sizeof(RSCommand) == 40);

struct TMessage {
    static constexpr EMessageType MessageType = EMessageType::PROTOCAL;
    uint32_t Type;
    uint32_t Len;
    uint32_t Src = 0;   // equiv to "node" in lab4
    uint32_t Dst = 0;
};

static_assert(sizeof(TMessage) == 16);

struct TLogEntry: public TMessage {
    static constexpr EMessageType MessageType = EMessageType::LOG_ENTRY;
    Command command;
};


struct Timestamp {
    uint32_t idx;
    uint32_t node_id;
};

struct TReplicate : public TMessage {
    static constexpr EMessageType MessageType = EMessageType::REPLICATE;
    Timestamp msg_id;
    bool operator< (TReplicate &other)
    {
        return msg_id.idx > other.msg_id.idx || 
            (msg_id.idx == other.msg_id.idx && msg_id.node_id > other.msg_id.node_id);
    }
};

struct TProposal : public TMessage {
    static constexpr EMessageType MessageType = EMessageType::PROPOSAL;
    Timestamp msg_id;
    uint64_t log_idx;
    Command command;
};
static_assert(sizeof(TProposal) == sizeof(TMessage) + 48);

struct TStateMsg : public TMessage {
    static constexpr EMessageType MessageType = EMessageType::STATE;
    uint64_t log_idx;
    RSCommand rsComand;

    TStateMsg(uint64_t seq, uint16_t r, EStateType est, Command c)
    {
        RSCommand rsc{
            .round = r,
            .state = est,
            .command = c
        };
        log_idx = seq;
        rsComand = rsc;
    };
};

struct TVote : public TMessage {
    static constexpr EMessageType MessageType = EMessageType::VOTE;
    uint64_t log_idx;
    RVCommand rvCommand;

    TVote(uint64_t seq, uint16_t r, EVoteType evt, Command c)
    {
        RVCommand rvc {
            .round = r,
            .vote = evt,
            .command = c
        };
        log_idx = seq;
        rvCommand = rvc;
    }

};

struct TDecided : public TMessage {
    static constexpr EMessageType MessageType = EMessageType::DECIDED;
    uint64_t log_idx;
    Command command;
};

struct TResponse : public TMessage{
    static constexpr EMessageType MessageType = EMessageType::RESPONSE;
    uint64_t client_seq;
    uint64_t value;
};