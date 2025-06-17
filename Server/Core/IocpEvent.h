#pragma once

class Session;

// IocpEvent 구조체는 OVERLAPPED를 확장하여 IOCP 이벤트를 처리합니다.
enum class EventType
{
    Recv,
    Send,
    Accept
};

struct IocpEvent : public OVERLAPPED
{
    EventType type;             // 이벤트 타입
    DWORD bytesTransferred = 0;     // 전송된 바이트 수

    // 초기화 및 재사용 시 호출
    void clear()
    {
        ZeroMemory(this, sizeof(IocpEvent));
        bytesTransferred = 0;
    }

    IocpEvent(EventType type) : type(type)
    {
        clear();
    }
};

struct RecvEvent : public IocpEvent
{
    std::shared_ptr<Session> session;

    RecvEvent() : IocpEvent(EventType::Recv) {}
};

struct SendEvent : public IocpEvent
{
    std::shared_ptr<Session> session;
    std::vector<char> buffer;

    SendEvent(const char* data, int len) : IocpEvent(EventType::Send), buffer(data, data + len) {}
};

struct AcceptEvent : public IocpEvent
{
    SOCKET clientSocket = INVALID_SOCKET;
    
    AcceptEvent() : IocpEvent(EventType::Accept) {}
};
