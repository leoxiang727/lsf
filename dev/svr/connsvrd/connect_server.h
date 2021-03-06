// File:        connect_server.h
// Description: ---
// Notes:       ---
// Author:      leoxiang <leoxiang727@qq.com>
// Revision:    2015-06-22 by leoxiang

#include "lsf/basic/singleton.hpp"
#include "svr/common/basic_server.h"

////////////////////////////////////////////////////////////
// ConnectServer
class ConnectServer : public BasicServer, public lsf::basic::Singleton<ConnectServer> {
public:
    ConnectServer() : BasicServer(conf::SERVER_TYPE_CONN_SERVER) {}

public:
    virtual bool OnRun();
};

// vim:ts=4:sw=4:et:ft=cpp:
