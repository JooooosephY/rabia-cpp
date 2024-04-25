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
    TMessage Read (TCmdReq message, uint64_t index) override;
    void Write(Command, uint64_t index) override;
    Command Prepare(TCmdReq message) override;

    private:
    uint64_t lastApplied;
    std::vector<Command> Log;
};

using TNodeDict = std::unordered_map<uint32_t, std::shared_ptr<INode>>;

struct RabiaState {

};
