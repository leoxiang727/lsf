// File:        basic_socket.hpp
// Description: ---
// Notes:       
// Author:      leoxiang <leoxiang727@qq.com>
// Revision:    2012-06-07 by leoxiang

#pragma once

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "lsf/basic/error.hpp"
#include "lsf/asio/detail/basic_sockaddr.hpp"

namespace lsf {
namespace asio {
namespace detail {

// forward declare
template<typename TransLayerProtocol>
class BasicListenSocket;

////////////////////////////////////////////////////////////
// DummyProtocol
////////////////////////////////////////////////////////////
class DummyProtocol
{
public:
    typedef void net_layer_proto;
    static DummyProtocol V4() { return DummyProtocol(); }
    static DummyProtocol V6() { return DummyProtocol(); }
    int domain()   const { return 0; }
    int type()     const { return 0; }
    int protocol() const { return 0; }
};

////////////////////////////////////////////////////////////
// BasicSocket
////////////////////////////////////////////////////////////
template<typename TransLayerProtocol = DummyProtocol>
class BasicSocket : public basic::Error
{
public:
    typedef BasicSockAddr<TransLayerProtocol>  sockaddr_type;
    typedef TransLayerProtocol                 proto_type;

    static const size_t MAX_BUFFER_LEN = 128 * 1024;

    template<typename TransLayerProtocol2>
    friend class BasicListenSocket;

public:
    static BasicSocket CreateSocket(proto_type proto = proto_type::V4()) 
    {
        int sockfd = ::socket(proto.domain(), proto.type(), proto.protocol());
        return BasicSocket(sockfd);
    }

    BasicSocket(int sockfd) : _sockfd(sockfd) { }

    ////////////////////////////////////////////////////////////
    bool Bind(sockaddr_type const & local) {
        return ErrWrap(::bind(_sockfd, local.Data(), local.DataSize())) == 0;
    }

    bool Connect(sockaddr_type const & remote) {
        return ErrWrap(::connect(_sockfd, remote.Data(), remote.DataSize())) == 0;
    }

    bool  Close() {
        return ErrWrap(::close(_sockfd)) == 0;
    }

    ssize_t Send(void const * buf, size_t len) {
        return ErrWrap(::send(_sockfd, buf, len, MSG_NOSIGNAL));
    }

    ssize_t Recv(void * buf, size_t len) {
        return ErrWrap(::recv(_sockfd, buf, len, 0));
    }

    bool RecvAll(std::string & content)
    {
        content.clear();

        while (true)
        {
            // recv
            static char buffer[MAX_BUFFER_LEN];
            int ret = Recv(buffer, sizeof(buffer));

            // handle error
            if (ret < 0)
            {
                if (errno == EINTR) // signal interrupt
                {
                    continue;
                }
                else if (errno == EAGAIN || errno == EWOULDBLOCK)   // no data
                {
                    break;
                }
                else // error
                {
                    break;
                }
            }

            // handle socket close
            if (ret == 0) break;

            // handle recv data
            content.append(buffer, ret);
        }
        
        return !content.empty();
    }

    bool SendAll(void const * buf, size_t len)
    {
        size_t total = 0;
        while (total < len)
        {
            int ret = Send((char const *)buf + total, len - total);

            // handle error
            if (ret < 0)
            {
                if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) // signal interrupt or not ready
                {
                    continue;
                }
                else // error
                {
                    break;
                }
            }

            // handle send data
            total += ret;
        }

        return total == len;
    }

    bool SendAll(std::string const & content)
    {
        return SendAll(content.data(), content.size());
    }

    ////////////////////////////////////////////////////////////
    // async funcs
    template<typename ServiceType, typename SockAddrType, typename HandlerType>
    bool AsyncConnect(ServiceType & io_service, SockAddrType const & sockaddr, HandlerType const & handler)
    {
        return io_service.AsyncConnect(*this, sockaddr, handler);
    }

    template<typename ServiceType, typename HandlerType>
    bool AsyncWrite(ServiceType & io_service, void const * buffer, size_t buflen, HandlerType const & handler)
    {
        return io_service.AsyncWrite(*this, buffer, buflen, handler);
    }

    template<typename ServiceType, typename HandlerType1, typename HandlerType2>
    bool AsyncRead(ServiceType & io_service, HandlerType1 const & read_handler, HandlerType2 const & rdhup_handler)
    {
        return io_service.AsyncRead(*this, read_handler, rdhup_handler);
    }

    template<typename ServiceType, typename HandlerType>
    bool AsyncRead(ServiceType & io_service, HandlerType const & handler)
    {
        return io_service.AsyncRead(*this, handler);
    }

    template<typename ServiceType>
    void CloseAsync(ServiceType & io_service)
    {
        io_service.CloseAsync(_sockfd);
    }

    void AsyncConnect();

    sockaddr_type LocalSockAddr() { 
        sockaddr addr;
        socklen_t   addrlen = sizeof(addr);
        ErrWrap(::getsockname(_sockfd, &addr, &addrlen));
        return sockaddr_type(&addr);
    }

    sockaddr_type RemoteSockAddr() {
        sockaddr addr;
        socklen_t   addrlen = sizeof(addr);
        if (ErrWrap(::getpeername(_sockfd, &addr, &addrlen)) == 0)
            return sockaddr_type(&addr);
        else if (IsV4())
            return sockaddr_type(proto_type::V4());
        else
            return sockaddr_type(proto_type::V6());
    }

    ////////////////////////////////////////////////////////////
    // SetSockOpt funcs
    bool SetNonBlock() {
        return ErrWrap(::fcntl(_sockfd, F_SETFL, ::fcntl(_sockfd, F_GETFL) | O_NONBLOCK)) == 0;
    }

    bool SetSendBuf(size_t buflen) {
        return ErrWrap(::setsockopt(_sockfd, SOL_SOCKET, SO_SNDBUF, &buflen, sizeof(buflen))) == 0;
    }

    bool SetRecvBuf(size_t buflen) {
        return ErrWrap(::setsockopt(_sockfd, SOL_SOCKET, SO_RCVBUF, &buflen, sizeof(buflen))) == 0;
    }

    bool SetSockReuse() {
        size_t optval = 1;
        return ErrWrap(::setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval))) == 0;
    }

    bool SetSendTimeout(timeval const & tv) {
        return ErrWrap(::setsockopt(_sockfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv))) == 0;
    }

    bool SetRecvTimeout(timeval const & tv) {
        return ErrWrap(::setsockopt(_sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv))) == 0;
    }

    bool SetLinger(bool is_on, int sec) {
        struct linger linger;
        linger.l_onoff = is_on;
        linger.l_linger = sec;
        return ErrWrap(::setsockopt(_sockfd, SOL_SOCKET, SO_LINGER, (char const *)&linger, sizeof(linger))) == 0;
    }

    size_t GetRecvBufLen() const {
        size_t optval;
        socklen_t optlen;
        ::getsockopt(_sockfd, SOL_SOCKET, SO_RCVBUF, &optval, &optlen);
        return optval;
    }

    size_t GetSendBufLen() const {
        size_t optval;
        socklen_t optlen;
        ::getsockopt(_sockfd, SOL_SOCKET, SO_SNDBUF, &optval, &optlen);
        return optval;
    }

    int  GetSockFd() const { return _sockfd; }

    bool IsV4() { return LocalSockAddr().IsV4(); }
    bool IsV6() { return LocalSockAddr().IsV6(); }

    bool operator!() const { return _sockfd >= 0; }

private:
    int             _sockfd;
};

} // end of namespace detail
} // end of namespace asio
} // end of namespace lsf

// vim:ts=4:sw=4:et:ft=cpp:
