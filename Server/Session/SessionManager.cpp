#include "pch.h"
#include "Session/SessionManager.h"
#include "Session/Session.h"

std::unordered_map<SOCKET, std::shared_ptr<Session>> SessionManager::s_sessions;
std::mutex SessionManager::s_mutex;

std::shared_ptr<Session> SessionManager::createSession(SOCKET socket)
{
    std::lock_guard<std::mutex> lock(s_mutex);

    if (s_sessions.find(socket) != s_sessions.end())
    {
        std::cerr << "SessionManager::createSession 실패: 이미 존재하는 소켓 = " << socket << std::endl;
        return nullptr;
    }

    std::shared_ptr<Session> newSession = std::make_shared<Session>(socket);
    s_sessions[socket] = newSession;
    return newSession;
}

void SessionManager::destroySession(std::shared_ptr<Session> session)
{
    std::lock_guard<std::mutex> lock(s_mutex);

    SOCKET socket = session->getSocket();
    auto it = s_sessions.find(socket);
    if (it != s_sessions.end())
    {
        s_sessions.erase(it);
        std::cout << "SessionManager::destroySession 성공: 소켓 = " << socket << std::endl;
    }
    else
    {
        std::cerr << "SessionManager::destroySession 실패: 존재하지 않는 소켓 = " << socket << std::endl;
    }
}
