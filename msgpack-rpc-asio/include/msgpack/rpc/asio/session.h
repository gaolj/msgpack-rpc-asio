#pragma once


namespace msgpack {
namespace rpc {
namespace asio {


class request_factory
{
    ::msgpack::rpc::msgid_t m_next_msgid;
public:
    request_factory()
        : m_next_msgid(1)
        {
        }

    ::msgpack::rpc::msgid_t next_msgid()
    {
        return m_next_msgid++;
    }

    // 0
    ::msgpack::rpc::msg_request<std::string, std::tuple<>> 
        create(const std::string &method)
        {
            ::msgpack::rpc::msgid_t msgid = next_msgid();
            typedef std::tuple<> Parameter;
            return ::msgpack::rpc::msg_request<std::string, Parameter>(
                    method, Parameter(), msgid);
        }
    // 1
    template<typename A1>
        ::msgpack::rpc::msg_request<std::string, std::tuple<A1>> 
        create(const std::string &method, A1 a1)
        {
            ::msgpack::rpc::msgid_t msgid = next_msgid();
            typedef std::tuple<A1> Parameter;
            return ::msgpack::rpc::msg_request<std::string, Parameter>(
                    method, Parameter(a1), msgid);
        }
    // 2
    template<typename A1, typename A2>
        ::msgpack::rpc::msg_request<std::string, std::tuple<A1, A2>> 
        create(const std::string &method, A1 a1, A2 a2)
        {
            ::msgpack::rpc::msgid_t msgid = next_msgid();
            typedef std::tuple<A1, A2> Parameter;
            return ::msgpack::rpc::msg_request<std::string, Parameter>(
                    method, Parameter(a1, a2), msgid);
        }
    // 3
    template<typename A1, typename A2, typename A3>
        ::msgpack::rpc::msg_request<std::string, std::tuple<A1, A2, A3>> 
        create(const std::string &method, A1 a1, A2 a2, A3 a3)
        {
            ::msgpack::rpc::msgid_t msgid = next_msgid();
            typedef std::tuple<A1, A2, A3> Parameter;
            return ::msgpack::rpc::msg_request<std::string, Parameter>(
                    method, Parameter(a1, a2, a3), msgid);
        }
    // 4
    template<typename A1, typename A2, typename A3, typename A4>
        ::msgpack::rpc::msg_request<std::string, std::tuple<A1, A2, A3, A4>> 
        create(const std::string &method, A1 a1, A2 a2, A3 a3, A4 a4)
        {
            ::msgpack::rpc::msgid_t msgid = next_msgid();
            typedef std::tuple<A1, A2, A3, A4> Parameter;
            return ::msgpack::rpc::msg_request<std::string, Parameter>(
                    method, Parameter(a1, a2, a3, a4), msgid);
        }
};


class func_call_error: public std::runtime_error
{
public:
    func_call_error(const std::string &msg)
        : std::runtime_error(msg)
    {}
};


class func_call
{
public:
    enum STATUS_TYPE
    {
        STATUS_WAIT,
        STATUS_RECEIVED,
        STATUS_ERROR,
    };
private:
    STATUS_TYPE m_status;
    error_code m_error_code;
    std::string m_error_msg;
    ::msgpack::object m_result;
    std::string m_request;
    boost::mutex m_mutex;
    boost::condition_variable_any m_cond;

    std::function<void(func_call*)> m_callback;
public:
    func_call(const std::string &s, std::function<void(func_call*)> callback)
        : m_status(STATUS_WAIT), m_request(s), m_error_code(success), m_callback(callback)
        {
        }

    void set_result(const ::msgpack::object &result)
    {
        if(m_status!=STATUS_WAIT){
            throw func_call_error("already finishded");
        }
        boost::mutex::scoped_lock lock(m_mutex);
        m_result=result;
        m_status=STATUS_RECEIVED;
        notify();
    }

    void set_error(const ::msgpack::object &error)
    {
        if(m_status!=STATUS_WAIT){
            throw func_call_error("already finishded");
        }
        boost::mutex::scoped_lock lock(m_mutex);
        typedef std::tuple<int, std::string> CodeWithMsg;
        CodeWithMsg codeWithMsg;
        error.convert(&codeWithMsg);
        m_status=STATUS_ERROR;
        m_error_code=static_cast<error_code>(std::get<0>(codeWithMsg));
        m_error_msg=std::get<1>(codeWithMsg);
        notify();
    }

    bool is_error()const{ return m_status==STATUS_ERROR; }

    error_code get_error_code()const{ 
        if(m_status!=STATUS_ERROR){
            throw func_call_error("no error !");
        }
        return m_error_code;
    }

    // blocking
    func_call& sync()
    {
        boost::mutex::scoped_lock lock(m_mutex);
        if(m_status==STATUS_WAIT){
            m_cond.wait(m_mutex);
        }
        return *this;
    }

    const ::msgpack::object &get_result()const
    {
        if(m_status==STATUS_RECEIVED){
            return m_result;
        }
        else{
            throw func_call_error("not ready");
        }
    }

    template<typename R>
        R& convert(R *value)const
        {
            if(m_status==STATUS_RECEIVED){
                m_result.convert(value);
                return *value;
            }
            else{
                throw func_call_error("not ready");
            }
        }

    std::string string()const
    {
        std::stringstream ss;
        ss << m_request << " = ";
        switch(m_status)
        {
            case func_call::STATUS_WAIT:
                ss << "?";
                break;
            case func_call::STATUS_RECEIVED:
                ss << m_result;
                break;
            case func_call::STATUS_ERROR:
                ss << "!";
                break;
            default:
                ss << "!?";
                break;
        }

        return ss.str();
    }

private:
    void notify()
    {
        if(m_callback){
            m_callback(this);
        }
        m_cond.notify_all();
    }
};
typedef std::function<void(func_call*)> func_call_callback_t;
inline std::ostream &operator<<(std::ostream &os, const func_call &request)
{
    os << request.string();
    return os;
}


class client_error: public std::runtime_error
{
public:
    client_error(const std::string &msg)
        : std::runtime_error(msg)
    {}
};


class Session
{
	boost::asio::io_service &m_io_service;
    request_factory m_request_factory;

    std::shared_ptr<Connection> m_connection;
    std::map<msgpack::rpc::msgid_t, std::shared_ptr<func_call>> m_request_map;
	std::shared_ptr<msgpack::rpc::asio::dispatcher> m_dispatcher;

    connection_callback_t m_connection_callback;
    error_handler_t m_error_handler;
	typedef std::function<void(const object &msg, std::shared_ptr<Connection> connection)> on_receive_t;

public:
	Session(boost::asio::io_service &io_service,
            connection_callback_t connection_callback=connection_callback_t(),
            error_handler_t error_handler=error_handler_t())
        : m_io_service(io_service), 
        m_connection_callback(connection_callback),
        m_error_handler(error_handler)
    {
	}

    void connect_async(const boost::asio::ip::tcp::endpoint &endpoint)
    {
		auto on_read = [this](const object &msg, std::shared_ptr<Connection> connection)
		{
			receive(msg, connection);
		};
		m_connection = Connection::create(m_io_service, on_read, m_connection_callback);
		m_connection->connect_async(endpoint);
    }

	void accept(std::shared_ptr<boost::asio::ip::tcp::socket> socket)
	{
		auto on_read = [this](const object &msg, std::shared_ptr<Connection> connection)
		{
			receive(msg, connection);
		};
		m_connection = Connection::create(m_io_service, on_read, m_connection_callback);
		m_connection->accept(socket);
	}
	
	void close()
    {
		m_connection->close();
    }

    bool is_connect()
    {
        return m_connection->get_connection_status()==connection_connected;
    }

	void set_dispatcher(std::shared_ptr<msgpack::rpc::asio::dispatcher> disp)
	{
		m_dispatcher = disp;
	}

	// call_async
    template<typename... A1>
    std::shared_ptr<func_call> call_async(func_call_callback_t callback, const std::string &method, A1... a1)
    {
        auto request=m_request_factory.create(method, a1...);
        return send_async(request, callback);
    }

    template<typename... A1>
    std::shared_ptr<func_call> call_async(const std::string &method, A1... a1)
    {
        auto request=m_request_factory.create(method, a1...);
        return send_async(request);
    }

    // call_sync
    template<typename... A1>
    void call_sync_void(const std::string &method, A1... a1)
    {
        auto request=m_request_factory.create(method, a1...);
        auto call=send_async(request);
        call->sync();
    }

    template<typename R, typename... A1>
    R &call_sync(R *value, const std::string &method, A1... a1)
    {
        auto request=m_request_factory.create(method, a1...);
        auto call=send_async(request);
        call->sync().convert(value);
        return *value;
    }

private: 
    template<typename Parameter>
        std::shared_ptr<func_call> send_async(
                const ::msgpack::rpc::msg_request<std::string, Parameter> &msgreq,
                func_call_callback_t callback=func_call_callback_t())
        {
            auto sbuf=std::make_shared<msgpack::sbuffer>();
            ::msgpack::pack(*sbuf, msgreq);

            std::stringstream ss;
            ss << msgreq.method << msgreq.param;
            auto req=std::make_shared<func_call>(ss.str(), callback);
            m_request_map.insert(std::make_pair(msgreq.msgid, req));

			m_connection->write_async(sbuf);

            return req;
        }

private:
	void receive(const object &msg, std::shared_ptr<Connection> connection)
	{
        ::msgpack::rpc::msg_rpc rpc;
        msg.convert(&rpc);
        switch(rpc.type) {
            case ::msgpack::rpc::REQUEST: 
				if (m_dispatcher)
					m_dispatcher->dispatch(msg, connection);
                break;

            case ::msgpack::rpc::RESPONSE: 
                {
                    ::msgpack::rpc::msg_response<object, object> res;
                    msg.convert(&res);
                    auto found=m_request_map.find(res.msgid);
                    if(found!=m_request_map.end()){
                        if(res.error.type==msgpack::type::NIL){
                            found->second->set_result(res.result);
                        }
                        else if(res.error.type==msgpack::type::BOOLEAN){
							bool isError;
							res.error.convert(&isError);
							if(isError){
								found->second->set_error(res.result);
							}
							else{
								found->second->set_result(res.result);
							}
                        }
                    }
                    else{
                        throw client_error("no request for response");
                    }
                }
                break;

            case ::msgpack::rpc::NOTIFY: 
                {
                    ::msgpack::rpc::msg_notify<object, object> req;
                    msg.convert(&req);
                }
                break;

            default:
                throw client_error("rpc type error");
        }
	}
};


}}}

