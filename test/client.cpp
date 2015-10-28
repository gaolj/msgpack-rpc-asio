#include <boost/test/unit_test.hpp> 

#include <msgpack/rpc/asio.h>
#include <boost/thread.hpp>

#include "fixture.h"

BOOST_AUTO_TEST_CASE( client )
{
	//_CrtSetBreakAlloc(1164);

	const static int PORT = 8070;

    Fixture fixture(PORT);

    // client
    boost::asio::io_service client_io;
	// avoid stop client_io when client closed
	boost::asio::io_service::work work(client_io);

	msgpack::rpc::asio::Client client(client_io);

    client.connect_async(boost::asio::ip::tcp::endpoint(
                boost::asio::ip::address::from_string("127.0.0.1"), PORT));
    boost::thread client_thread([&client_io](){ client_io.run(); });

    // request
    int result1;
    BOOST_CHECK_EQUAL(client.call_sync(&result1, "add", 1, 2), 3);

    auto request2=client.call_async("add", 3, 4);
    request2->sync();
    int result2;
    /*
    BOOST_CHECK_EQUAL(request2->convert(&result2), 7);
    */
    // raw extract
    const msgpack::object &obj=request2->get_result();
    obj.convert(&result2);
    BOOST_CHECK_EQUAL(result2, 7);

    // close
    client.close();
    // no active socket.

    // reconnect
    client.connect_async(boost::asio::ip::tcp::endpoint(
                boost::asio::ip::address::from_string("127.0.0.1"), PORT));

    auto request3=client.call_async("add", 5, 6);
    request3->sync();
    int result3;
    BOOST_CHECK_EQUAL(request3->convert(&result3), 11);

    // close
    client_io.stop();
    client_thread.join();
}

