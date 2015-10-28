#pragma once

struct Fixture 
{
    // dispatcher
    boost::asio::io_service dispatcher_io;
	boost::asio::io_service::work work;
	msgpack::rpc::asio::dispatcher dispatcher;
    std::shared_ptr<boost::thread> dispatcher_thread;

    boost::asio::io_service server_io;
	msgpack::rpc::asio::RpcServer server;
    std::shared_ptr<boost::thread> server_thread;
    boost::mutex m_mutex;

    Fixture(int port) 
        : server(server_io), work(dispatcher_io)
    {
		std::shared_ptr<msgpack::rpc::asio::dispatcher> dispatcher = std::make_shared<msgpack::rpc::asio::dispatcher>();
		dispatcher->add_handler("zero", &Fixture::zero);
        dispatcher->add_handler("acc", &Fixture::unary);
        dispatcher->add_handler("add", &Fixture::binary);
        dispatcher->add_handler("add3", [](int a, int b, int c){ return a+b+c; });
        dispatcher->add_handler("add4", [](int a, int b, int c, int d){ return a+b+c+d; });

		auto &mutex=m_mutex;
        auto error_handler=[&mutex](boost::system::error_code error)
        {
			if(error==boost::asio::error::connection_reset){
				// closed
				return;
			}
            boost::mutex::scoped_lock lock(mutex);
			auto msg=error.message();
            std::cerr << msg << std::endl;
        };
        server.set_error_handler(error_handler);
		server.set_dispatcher(dispatcher);

        server.listen(boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port));
        server_thread=std::make_shared<boost::thread>([&]{ server_io.run(); });
    }   
    ~Fixture() {
        dispatcher_io.stop();
        server_io.stop();
        server_thread->join();
    }   

    boost::mutex &mutex(){ return m_mutex; }

    static int zero()
    {
        return 0;
    }
    static int unary(int a)
    {
        static int acc=0;
        acc+=a;
        return acc;
    }
    static int binary(int a, int b)
    {
        return a+b;
    }
};

