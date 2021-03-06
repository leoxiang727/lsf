// File:        session_manager.cpp
// Description: ---
// Notes:       ---
// Author:      leoxiang <leoxiang727@qq.com>
// Revision:    2015-06-15 by leoxiang

#include "svr/common/session_manager.h"
#include "svr/common/common_header.h"
#include "svr/common/timer_manager.h"
#include "svr/common/basic_handler.h"
#include "svr/common/handler_manager.h"

using namespace lsf::basic;
using namespace lsf::util;
using namespace lsf::asio;

////////////////////////////////////////////////////////////
// Session
bool Session::Serialize(void *buf, size_t buflen, size_t& uselen) const {
    if (!lsf::util::SerializeProtobuf(buf, buflen, uselen, *this)) return false;
    return true;
}

bool Session::UnSerialize(void *buf, size_t buflen, size_t& uselen) {
    if (!lsf::util::UnSerializeProtobuf(buf, buflen, uselen, *this)) return false;
    return true;
}

std::string Session::ToString() const {
    static std::string tmp;
    tmp = "sess_id=" + TypeCast<std::string>(session_id());
    if (cs_request().has_msg_type())
        tmp += ", player_uid=" + TypeCast<std::string>(cs_request().conn_head().uid()) +
               ", conn_id=" + TypeCast<std::string>(cs_request().conn_head().conn_id());
    if (ss_request().has_msg_type() && ss_request().proxy_head().has_src_server_type())
        tmp += ", server_type=" + ProtoEnumToString(ss_request().proxy_head().src_server_type()) +
               ", server_id=" + TypeCast<std::string>(ss_request().proxy_head().src_server_id());
    return tmp;
}

char const* Session::GetType() const {
    if (cs_request().has_msg_type()) return LSF_ETS(cs_request().msg_type());
    if (ss_request().has_msg_type()) return LSF_ETS(ss_request().msg_type());
    return "";
}

////////////////////////////////////////////////////////////
// Session Manager
bool SessionManager::Init(key_t shm_key, uint32_t max_size) {
    // call base init
    if (!base_type::Init(shm_key, max_size)) return false;

    // recover max id
    _cur_max_id = 0;
    for (auto& pair : *this) { _cur_max_id = std::max(_cur_max_id, pair.second.session_id()); }

    // register session timeout handle
    TimerManager::Instance()->AddTimerHandle(data::TIMER_TYPE_SESSION_TIMEOUT, [this](Timer const& timer) {
            Session* psession = GetSession(timer.session_id());
            if (psession) HandlerManager::Instance(*psession)->WaitResponseTimeout(*psession);
        });

    // register session check leak
    IOService::Instance()->AsyncAddTimerForever(DEF_SESSION_LEAK_CHECK_INTERVAL, [this](int) {
            // find leak session
            std::vector<uint32_t> del_vec;
            for (auto& pair : *this) {
                auto& session = pair.second;
                if (session.create_time() + DEF_SESSION_LEAK_TIME < IOService::Instance()->ClockTimeMilli()) {
                    LSF_LOG_ERR("detect session leak, %s", LSF_TS(session));
                    del_vec.push_back(session.session_id());
                }
            }
            // release leak session
            for (auto session_id : del_vec) ReleaseSession(session_id);
        });

    return true;
}

Session* SessionManager::CreateSession() {
    // check full
    if (base_type::full()) {
        LSF_LOG_ERR("session is full, size=%u, max_size=%u", base_type::size(), base_type::max_size());
        return nullptr;
    }

    // find a valid id
    uint32_t session_id;
    do { session_id = ++_cur_max_id& MAX_SESSION_ID; } while (base_type::exist(session_id));

    // create session
    Session& session = base_type::operator[](session_id);

    // init
    session.set_session_id(session_id);
    session.set_create_time(IOService::Instance()->ClockTimeMilli());

    return &session;
}

void SessionManager::ReleaseSession(uint32_t session_id) {
    // release timer
    Session* psession = GetSession(session_id);
    if (psession && psession->has_timer_id()) TimerManager::Instance()->ReleaseTimer(psession->timer_id());

    // free object
    base_type::erase(session_id);
}

Session* SessionManager::GetSession(uint32_t session_id) {
    auto iter = base_type::find(session_id);
    return iter == base_type::end() ? nullptr : &iter->second;
}

// vim:ts=4:sw=4:et:ft=cpp:
