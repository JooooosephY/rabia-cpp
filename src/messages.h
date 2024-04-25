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
    CMD_REQ = 1,
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

    bool operator== (const Command &other)
    {
        return operation == other.operation &&
                key == other.key && value == other.value;
    }
};

struct CommandHash {
    std::size_t operator()(const Command& cmd) const {
        std::size_t seed = 0;
        seed ^= std::hash<uint64_t>()(cmd.client_seq) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<uint32_t>()(static_cast<uint32_t>(cmd.operation)) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<uint64_t>()(cmd.key) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<uint64_t>()(cmd.value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }
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

static_assert(sizeof(GetCommand) == 32);

struct RSCommand {
    uint16_t round;
    EStateType state;
    Command command;
    bool operator== (RSCommand &other)
    {
        return round == other.round && state == other.state
                && command == other.command;
    }
};  // name stands for: round state command

struct RSCommandHash {
    std::size_t operator()(const RSCommand& rsCmd) const {
        std::size_t seed = 0;
        seed ^= std::hash<uint16_t>()(rsCmd.round) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<EStateType>()(rsCmd.state) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= CommandHash()(rsCmd.command) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }
};

struct RVCommand {
    uint16_t round;
    EVoteType vote;
    Command command;
};  // name stands for: round vote command


static_assert(sizeof(RSCommand) == 40);

// Base Struct TMessage
// size 16
struct TMessage { 
    static constexpr EMessageType MessageType = EMessageType::PROTOCAL;
    uint32_t Type;
    uint32_t Len;
    uint32_t Src = 0;   // equiv to "node" in lab4
    uint32_t Dst = 0;
};

// Client message
// size 48
struct TCmdReq: public TMessage {
    static constexpr EMessageType MessageType = EMessageType::CMD_REQ;
    Command command;
};
static_assert(sizeof(TCmdReq) == 48);


struct Timestamp {
    uint32_t idx;
    uint32_t node_id;
};

// Equiv to :replicate 
// size 56
struct TReplicate : public TMessage {
    static constexpr EMessageType MessageType = EMessageType::REPLICATE;
    Timestamp msg_id;
    Command command;
    bool operator< (TReplicate &other)
    {
        return msg_id.idx > other.msg_id.idx || 
            (msg_id.idx == other.msg_id.idx && msg_id.node_id > other.msg_id.node_id);
    }
};

// Equiv to :proposal
// size 64
struct TProposal : public TMessage {
    static constexpr EMessageType MessageType = EMessageType::PROPOSAL;
    Timestamp msg_id;
    uint64_t log_idx;
    Command command;
};
static_assert(sizeof(TProposal) == 64);


// Equiv to :state
// size 64
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

// Equiv to :vote
// size 64
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

// Decided
// size 56
struct TDecided : public TMessage {
    static constexpr EMessageType MessageType = EMessageType::DECIDED;
    uint64_t log_idx;
    Command command;
};
static_assert(sizeof(TDecided) == 56);


// Response to Client
// size 32
struct TResponse : public TMessage{
    static constexpr EMessageType MessageType = EMessageType::RESPONSE;
    uint64_t client_seq;
    uint64_t value;
};