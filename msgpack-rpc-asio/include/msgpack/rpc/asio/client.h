#pragma once

namespace msgpack {
namespace rpc {
namespace asio {

class Client
{
	boost::asio::io_service &m_io_service;
	std::shared_ptr<Session> m_session;


public:
	Client(boost::asio::io_service &io_service)
	: m_io_service(io_service)
	{
	}

	void connect_async(const boost::asio::ip::tcp::endpoint &endpoint)
	{
		m_session = std::make_shared<Session>(m_io_service);
		m_session->connect_async(endpoint);
	}

	void close()
	{
		m_session->close();
	}

	bool is_connect()
	{
		return m_session->is_connect();
	}

	void set_dispatcher(std::shared_ptr<msgpack::rpc::asio::dispatcher> disp)
	{
		m_session->set_dispatcher(disp);
	}

	// call_async
	template<typename... A1>
	std::shared_ptr<func_call> call_async(func_call_callback_t callback, const std::string &method, A1... a1)
	{
		return m_session->call_async(callback, method, a1...);
	}

	template<typename... A1>
	std::shared_ptr<func_call> call_async(const std::string &method, A1... a1)
	{
		return m_session->call_async(method, a1...);
	}

	// call_sync
	template<typename... A1>
	void call_sync_void(const std::string &method, A1... a1)
	{
		m_session->call_sync_void(method, a1...);
	}

	template<typename R, typename... A1>
	R &call_sync(R *value, const std::string &method, A1... a1)
	{
		return m_session->call_sync(value, method, a1...);
	}

};

} } }