#pragma once
#include <mutex>
#include "session.h"

namespace msgpack {
namespace rpc {
namespace asio {

class SessionManager
{
public:
	/// Create instance
	static SessionManager* instance();

	/// Add the specified session to the manager and start it.
	void start(SessionPtr session);

	/// Stop the specified connection.
	void stop(SessionPtr session);

	/// Stop all session.
	void stopAll();

private:
	SessionManager();
	SessionManager(const SessionManager&) = delete;
	SessionManager& operator=(const SessionManager&) = delete;
	
	std::mutex _mtx;
	std::set<SessionPtr> _sessionPool;
};

inline SessionManager* SessionManager::instance()
{
	static std::once_flag instance_flag;
	static SessionManager* m_pInstance;

	std::call_once(instance_flag, [](){ m_pInstance = new SessionManager(); });
	return m_pInstance;
}

inline SessionManager::SessionManager()
{
}

inline void SessionManager::start(SessionPtr session)
{
	std::unique_lock<std::mutex> lck(_mtx);
	_sessionPool.insert(session);
	//c->start();
}

inline void SessionManager::stop(SessionPtr session)
{
	std::unique_lock<std::mutex> lck(_mtx);
	_sessionPool.erase(session);
	//c->stop();
}

inline void SessionManager::stopAll()
{
	std::unique_lock<std::mutex> lck(_mtx);
	for (auto session : _sessionPool)
		session->stop();
	_sessionPool.clear();
}

} } }