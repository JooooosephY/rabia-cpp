#include <chrono>
#include <coroutine>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <vector>
#include "rabia.h"
#include "server.h"
#include "messages.h"

template<typename TSocket>
NNet::TValueTask<void> TMessageWriter<TSocket>::Write(TMessage message) {
    co_await NNet::TByteWriter(Socket).Write(message, message.Len);

    // auto payload = std::move(message.Payload);
    // for (uint32_t i = 0; i < message.PayloadSize; ++i) {
    //     co_await Write(std::move(payload[i]));
    // }
    co_return;
}

template<typename TSocket>
NNet::TValueTask<TMessage> TMessageReader<TSocket>::Read() {
    decltype(TMessage::Type) type;
    decltype(TMessage::Len) len;
    auto s = co_await Socket.ReadSome(&type, sizeof(type));
    if (s != sizeof(type)) {
        throw std::runtime_error("Connection closed");
    }
    // s = co_await Socket.ReadSome(&len, sizeof(len));
    // if (s != sizeof(len)) {
    //     throw std::runtime_error("Connection closed");
    // }
    switch (type){
        case 0:
        {
            auto structReader = NNet::TStructReader<TMessage, TSocket>(Socket);
            auto msg = co_await structReader.Read();
            co_return  msg;
        }
        case 1:
        {
            auto structReader = NNet::TStructReader<TCmdReq, TSocket>(Socket);
            auto msg = co_await structReader.Read();
            co_return msg;
        }
        case 2:
        {
            auto structReader = NNet::TStructReader<TReplicate, TSocket>(Socket);
            auto msg = co_await structReader.Read();
            co_return msg;
        }
        case 3:
        {
            auto structReader = NNet::TStructReader<TProposal, TSocket>(Socket);
            auto msg = co_await structReader.Read();
            co_return msg;
        }
        case 4:
        {
            auto structReader = NNet::TStructReader<TStateMsg, TSocket>(Socket);
            auto msg = co_await structReader.Read();
            co_return msg;
        }
        case 5:
        {
            auto structReader = NNet::TStructReader<TVote, TSocket>(Socket);
            auto msg = co_await structReader.Read();
            co_return msg;
        }
        case 6:
        {
            auto structReader = NNet::TStructReader<TDecided, TSocket>(Socket);
            auto msg = co_await structReader.Read();
            co_return msg;
        }
        case 7:
        {
            auto structReader = NNet::TStructReader<TResponse, TSocket>(Socket);
            auto msg = co_await structReader.Read();
            co_return msg;
        }
        default:
            break;
    }
}

template<typename TSocket>
void TNode<TSocket>::Send(TMessage message) {
    Messages.emplace_back(std::move(message));
}

template<typename TSocket>
void TNode<TSocket>::Drain() {
    if (!Connected) {
        Connect();
        return;
    }
    if (!Drainer || Drainer.done()) {
        if (Drainer && Drainer.done()) {
            Drainer.destroy();
        }
        Drainer = DoDrain();
    }
}

template<typename TSocket>
NNet::TVoidSuspendedTask TNode<TSocket>::DoDrain() {
    try {
        while (!Messages.empty()) {
            auto tosend = std::move(Messages); Messages.clear();
            for (auto&& m : tosend) {
                co_await TMessageWriter(Socket).Write(std::move(m));
            }
        }
    } catch (const std::exception& ex) {
        std::cout << "Error on write: " << ex.what() << "\n";
        Connect();
    }
    co_return;
}

template<typename TSocket>
void TNode<TSocket>::Connect() {
    if (Address && (!Connector || Connector.done())) {
        if (Connector && Connector.done()) {
            Connector.destroy();
        }
        Connector = DoConnect();
    }
}

template<typename TSocket>
NNet::TVoidSuspendedTask TNode<TSocket>::DoConnect() {
    std::cout << "Connecting " << Name << "\n";
    Connected = false;
    while (!Connected) {
        try {
            auto deadline = NNet::TClock::now() + std::chrono::milliseconds(100); // TODO: broken timeout in coroio
            Socket = SocketFactory(*Address);
            co_await Socket.Connect(deadline);
            std::cout << "Connected " << Name << "\n";
            Connected = true;
        } catch (const std::exception& ex) {
            std::cout << "Error on connect: " << Name << " " << ex.what() << "\n";
        }
        if (!Connected) {
            co_await Socket.Poller()->Sleep(std::chrono::milliseconds(1000));
        }
    }
    co_return;
}

template<typename TSocket>
NNet::TVoidTask TRabiaServer<TSocket>::InboundConnection(TSocket socket) {
    std::shared_ptr<TNode<TSocket>> client;
    try {
        client = std::make_shared<TNode<TSocket>>(
            "client", std::move(socket), TimeSource
        );
        Nodes.insert(client);
        while (true) {
            auto mes = co_await TMessageReader(client->Sock()).Read();
            Raft->Process(TimeSource->Now(), std::move(mes), client);
            Raft->ProcessTimeout(TimeSource->Now());
            DrainNodes();
        }
    } catch (const std::exception & ex) {
        std::cerr << "Exception: " << ex.what() << "\n";
    }
    Nodes.erase(client);
    co_return;
}

template<typename TSocket>
void TRabiaServer<TSocket>::Serve() {
    Idle();
    InboundServe();
}

template<typename TSocket>
void TRabiaServer<TSocket>::DrainNodes() {
    for (const auto& node : Nodes) {
        node->Drain();
    }
}

template<typename TSocket>
NNet::TVoidTask TRabiaServer<TSocket>::InboundServe() {
    while (true) {
        auto client = co_await Socket.Accept();
        std::cout << "Accepted\n";
        InboundConnection(std::move(client));
    }
    co_return;
}

template<typename TSocket>
void TRabiaServer<TSocket>::DebugPrint() {
    auto* state = Raft->GetState();
    auto* volatileState = Raft->GetVolatileState();
    if (Raft->CurrentStateName() == EState::LEADER) {
        std::cout << "Leader, "
            << "Term: " << state->CurrentTerm << ", "
            << "Index: " << state->Log.size() << ", "
            << "CommitIndex: " << volatileState->CommitIndex << ", ";
        std::cout << "Delay: ";
        for (auto [id, index] : volatileState->MatchIndex) {
            std::cout << id << ":" << (state->Log.size() - index) << " ";
        }
        std::cout << "MatchIndex: ";
        for (auto [id, index] : volatileState->MatchIndex) {
            std::cout << id << ":" << index << " ";
        }
        std::cout << "NextIndex: ";
        for (auto [id, index] : volatileState->NextIndex) {
            std::cout << id << ":" << index << " ";
        }
        std::cout << "\n";
    } else if (Raft->CurrentStateName() == EState::CANDIDATE) {
        std::cout << "Candidate, "
            << "Term: " << state->CurrentTerm << ", "
            << "Index: " << state->Log.size() << ", "
            << "CommitIndex: " << volatileState->CommitIndex << ", "
            << "\n";
    } else if (Raft->CurrentStateName() == EState::FOLLOWER) {
        std::cout << "Follower, "
            << "Term: " << state->CurrentTerm << ", "
            << "Index: " << state->Log.size() << ", "
            << "CommitIndex: " << volatileState->CommitIndex << ", "
            << "\n";
    }
}

template<typename TSocket>
NNet::TVoidTask TRabiaServer<TSocket>::Idle() {
    auto t0 = TimeSource->Now();
    auto dt = std::chrono::milliseconds(2000);
    auto sleep = std::chrono::milliseconds(100);
    while (true) {
        Raft->ProcessTimeout(TimeSource->Now());
        DrainNodes();
        auto t1 = TimeSource->Now();
        if (t1 > t0 + dt) {
            DebugPrint();
            t0 = t1;
        }
        co_await Poller.Sleep(t1 + sleep);
    }
    co_return;
}

template class TRabiaServer<NNet::TSocket>;
template class TRabiaServer<NNet::TSslSocket<NNet::TSocket>>;
#ifdef __linux__
template class TRabiaServer<NNet::TUringSocket>;
template class TRabiaServer<NNet::TSslSocket<NNet::TUringSocket>>;
#endif
