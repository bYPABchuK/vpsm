#pragma once

#include "../../application/ControlPlane/IControlRouter.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/system/error_code.hpp>

#include <cstdint>
#include <memory>

namespace vpsm::server::adapter::boostImpl {
    class HttpSessionBoost : public std::enable_shared_from_this<HttpSessionBoost> {
    public:
        HttpSessionBoost(
            boost::asio::ip::tcp::socket socket,
            std::shared_ptr<application::IControlRouter> router
        );

        void run();

    private:
        void doRead();
        void onRead(const boost::system::error_code& ec, std::size_t bytesTransferred);
        void onWrite(bool close, const boost::system::error_code& ec, std::size_t bytesTransferred);
        void doClose();

        boost::asio::ip::tcp::socket socket_;
        std::shared_ptr<application::IControlRouter> router_;
        boost::beast::flat_buffer buffer_;
        boost::beast::http::request<boost::beast::http::string_body> req_;
    };

    class HttpListenerBoost : public std::enable_shared_from_this<HttpListenerBoost> {
    public:
        HttpListenerBoost(
            boost::asio::io_context& io,
            std::shared_ptr<application::IControlRouter> router,
            std::uint16_t port
        );

        int start();
        int stop();

    private:
        void doAccept();
        void onAccept(const boost::system::error_code& ec, boost::asio::ip::tcp::socket socket);

        boost::asio::io_context& io_;
        std::shared_ptr<application::IControlRouter> router_;
        boost::asio::ip::tcp::acceptor acceptor_;
        boost::system::error_code initError_;
        bool started_ = false;
    };
}
