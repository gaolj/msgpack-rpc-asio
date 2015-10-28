#pragma once

namespace msgpack {
namespace rpc {
namespace asio {


class RpcServer
{
	typedef std::function<void(const object &msg, ConnectionPtr connection)> on_receive_t;

    boost::asio::io_service &m_io_service;
    boost::asio::ip::tcp::acceptor m_acceptor;
    std::set<SessionPtr> m_sessions;
	std::shared_ptr<msgpack::rpc::asio::dispatcher> m_dispatcher;

    on_receive_t m_on_receive;
    error_handler_t m_error_handler;
public:
	RpcServer(boost::asio::io_service &io_service)
		: m_io_service(io_service), m_acceptor(io_service)
	{
	}

	RpcServer(boost::asio::io_service &io_service, on_receive_t on_receive)
        : m_io_service(io_service), m_acceptor(io_service), m_on_receive(on_receive)
    {
    }

	RpcServer(boost::asio::io_service &io_service,
            on_receive_t on_receive, error_handler_t error_handler)
        : m_io_service(io_service), m_acceptor(io_service), 
        m_on_receive(on_receive), m_error_handler(error_handler)
    {
    }

	~RpcServer()
    {
    }

	void set_dispatcher(std::shared_ptr<msgpack::rpc::asio::dispatcher> disp)
	{
		m_dispatcher = disp;
	}

	void set_on_receive(on_receive_t on_receive)
	{
		m_on_receive=on_receive;
	}

    void set_error_handler(error_handler_t error_handler)
    {
        m_error_handler=error_handler;
    }

    void listen(boost::asio::ip::tcp::endpoint endpoint)
    {
        m_acceptor.open(endpoint.protocol());
        m_acceptor.bind(endpoint);
        m_acceptor.listen();
        start_accept();
    }

    void stop()
    {
        m_acceptor.close();
    }

	void finishSession(SessionPtr session)
	{
		auto iter = m_sessions.find(session);
		if (iter != m_sessions.end())
			m_sessions.erase(iter);
	}

private:
    void start_accept()
    {
		using std::placeholders::_1;
		SessionPtr session = std::make_shared<Session>(m_io_service, std::bind(&RpcServer::finishSession, this, _1));
		session->set_dispatcher(m_dispatcher);
        auto socket=std::make_shared<boost::asio::ip::tcp::socket>(m_io_service);


        auto self=this;
        auto on_accept=[self, session, socket/*keep socket*/](
                const boost::system::error_code& error){
            if (error){
                if(self->m_error_handler){
                    self->m_error_handler(error);
                }
                else{
                    throw error;
                }
            }
            else{
				self->m_sessions.insert(session);
				session->accept(socket);
                // next
                self->start_accept();
            }
        };
        m_acceptor.async_accept(*socket.get(), on_accept);
    }
};

}}}
