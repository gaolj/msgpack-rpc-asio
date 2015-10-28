#include <msgpack/rpc/asio.h>
#include <boost/thread.hpp>


class SomeClass
{
    int m_number;

public:
    void setNumber(const int &number){ m_number=number; }
    int getNumber()const{ return m_number; }
};


int main(int argc, char **argv)
{
    const static int PORT=8070;

    // dispatcher
	std::shared_ptr<msgpack::rpc::asio::dispatcher> dispatcher = std::make_shared<msgpack::rpc::asio::dispatcher>();;
    dispatcher->add_handler("add", [](int a, int b)->int{ return a+b; });
    dispatcher->add_handler("mul", [](float a, float b)->float{ return a*b; });
    SomeClass s;
	dispatcher->add_property("number", std::function<SomeClass*()>([&s](){ return &s; })
            , &SomeClass::getNumber
            , &SomeClass::setNumber
            );

    // server
    boost::asio::io_service server_io;
	msgpack::rpc::asio::RpcServer server(server_io);
	server.set_dispatcher(dispatcher);
    server.listen(boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), PORT));
    boost::thread server_thread([&server_io](){ server_io.run(); });

    // client
    boost::asio::io_service client_io;

	// avoid stop client_io when client closed
	boost::asio::io_service::work work(client_io);

    msgpack::rpc::asio::Client client(client_io); 
    client.connect_async(boost::asio::ip::tcp::endpoint(
                    boost::asio::ip::address::from_string("127.0.0.1"), PORT));
    boost::thread clinet_thread([&client_io](){ client_io.run(); });

    // sync request
	int result1;
    std::cout << "add, 1, 2 = " << client.call_sync(&result1, "add", 1, 2) << std::endl;

    // close
    client.close();

    // reconnect
    client.connect_async(boost::asio::ip::tcp::endpoint(
                boost::asio::ip::address::from_string("127.0.0.1"), PORT));

    // request callback
	auto on_result=[](msgpack::rpc::asio::func_call* result){
		int result2;
		std::cout << "add, 3, 4 = " << result->convert(&result2) << std::endl;
	};
    auto result2=client.call_async(on_result, "add", 3, 4);

	// block
	result2->sync();

    // stop asio
    client_io.stop();
    clinet_thread.join();

    server_io.stop();
    server_thread.join();

    return 0;
}
