------------------------------------------------------------------------------
-- Solution
------------------------------------------------------------------------------
solution "msgpack-rpc-asio"
configurations { "Debug", "Release" }

configuration "Debug"
do
	defines { "DEBUG" }
	flags { "Symbols", }
	targetdir "debug"
end

configuration "Release"
do
	defines { "NDEBUG" }
	flags { "Optimize", }
	targetdir "release"
end

configuration "gmake"
do
    buildoptions {
        "-std=c++0x",
        --"-Wno-deprecated",
    }
end

configuration {"gmake", "linux" }
do
    defines {
        "BOOST_TEST_DYN_LINK",
    }
    links {
        "boost_thread-mt",
        "boost_system-mt",
        "boost_unit_test_framework-mt",
    }
end

configuration { "gmake", "windows", "debug" }
do
    links {
        "boost_thread-mgw47-mt-d-1_54",
        "boost_system-mgw47-mt-d-1_54",
        "boost_unit_test_framework-mgw47-mt-d-1_54",
        "ws2_32",
        "Mswsock",
    }
end

configuration { "gmake", "windows", "release" }
do
    links {
        "boost_thread-mgw47-mt-1_54",
        "boost_system-mgw47-mt-1_54",
        "boost_unit_test_framework-mgw47-mt-1_54",
        "ws2_32",
        "Mswsock",
    }
end

configuration "Windows"
do
    includedirs {
        "D:\\Program Files\\boost_1_59_0",
    }
    defines {
        "_WIN32_WINNT=0x0501",
    }
    libdirs {
        "D:\\Program Files\\boost_1_59_0\\lib32-msvc-12.0",
    }
end

configuration "vs*"
do
    buildoptions {
        "/TP",
        "/wd4800",
    }
end

configuration {}

include "client"
include "test"
include "sample"
include "msgpack-rpc-asio"
include "msgpack"

