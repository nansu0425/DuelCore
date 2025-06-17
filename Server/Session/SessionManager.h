#pragma once

class Session;

// SessionManager 클래스는 세션 풀을 관리합니다.
class SessionManager
{
public:
    static std::shared_ptr<Session> createSession(SOCKET socket);
    static void destroySession(std::shared_ptr<Session> session);

private:
    static std::unordered_map<SOCKET, std::shared_ptr<Session>> s_sessions; // 세션 풀
    static std::mutex s_mutex; // 세션 풀 보호를 위한 뮤텍스
};
