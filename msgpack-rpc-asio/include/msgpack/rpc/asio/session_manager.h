#pragma once
#include "session.h"

namespace msgpack {
namespace rpc {
namespace asio {

class SessionManager
{
public:
	SessionManager(const SessionManager&) = delete;
	SessionManager& operator=(const SessionManager&) = delete;

	/// Construct a session manager.
	SessionManager();

	/// Add the specified session to the manager and start it.
	void start(SessionPtr session);

	/// Stop the specified connection.
	void stop(SessionPtr session);

	/// Stop all session.
	void stopAll();

private:
	/// The managed session.
	std::set<SessionPtr> _sessionPool;
};

inline SessionManager::SessionManager()
{
}

inline void SessionManager::start(SessionPtr session)
{
	connections_.insert(c);
	c->start();
}

inline void SessionManager::stop(SessionPtr session)
{
	connections_.erase(c);
	c->stop();
}

inline void SessionManager::stop_all()
{
	for (auto c : connections_)
		c->stop();
	connections_.clear();
}

} } }