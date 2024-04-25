#pragma once

#include <unordered_map>
#include <string>

#include <raft.h>

class TKv: public IRsm {
public:
    TMessageHolder<TMessage> Read(TMessageHolder<TCommandRequest> message, uint64_t index) override;
    void Write(TMessageHolder<TCmdReq> message, uint64_t index) override;
    TMessageHolder<TCmdReq> Prepare(TMessageHolder<TCommandRequest> message, uint64_t term) override;

private:
    uint64_t LastAppliedIndex = 0;
    std::unordered_map<std::string, std::string> H;
};
