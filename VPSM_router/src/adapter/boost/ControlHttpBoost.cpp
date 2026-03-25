#include "ControlHttpBoost.hpp"

#include <boost/asio/dispatch.hpp>
#include <boost/beast/core/bind_handler.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/vector_body.hpp>
#include <boost/beast/http/write.hpp>

namespace vpsm::server::adapter::boostImpl {
    namespace {
        namespace http = boost::beast::http;

        application::ControlRequest toControlRequest(const http::request<http::string_body>& req) {
            application::ControlRequest out;
            out.method = std::string(req.method_string());
            out.path = std::string(req.target());
            out.body.assign(req.body().begin(), req.body().end());

            for (const auto& field : req) {
                out.headers.emplace(std::string(field.name_string()), std::string(field.value()));
            }

            return out;
        }
    }

    HttpSessionBoost::HttpSessionBoost(
        boost::asio::ip::tcp::socket socket,
        std::shared_ptr<application::IControlRouter> router
    )
        : socket_(std::move(socket)),
          router_(std::move(router)) {}

    void HttpSessionBoost::run() {
        boost::asio::dispatch(
            socket_.get_executor(),
            boost::beast::bind_front_handler(&HttpSessionBoost::doRead, shared_from_this())
        );
    }

    void HttpSessionBoost::doRead() {
        req_ = {};
        http::async_read(
            socket_,
            buffer_,
            req_,
            boost::beast::bind_front_handler(&HttpSessionBoost::onRead, shared_from_this())
        );
    }

    void HttpSessionBoost::onRead(const boost::system::error_code& ec, std::size_t bytesTransferred) {
        (void)bytesTransferred;
        if (ec == http::error::end_of_stream) {
            doClose();
            return;
        }
        if (ec) {
            return;
        }

        const auto controlReq = toControlRequest(req_);
        const auto controlResp = router_->route(controlReq);

        auto resp = std::make_shared<http::response<http::vector_body<std::uint8_t>>>(
            static_cast<http::status>(controlResp.status),
            req_.version()
        );
        resp->set(http::field::server, "vpsm-control");
        resp->set(http::field::content_type, controlResp.contentType);
        resp->keep_alive(req_.keep_alive());
        resp->body() = controlResp.body;
        resp->prepare_payload();

        const bool close = resp->need_eof();
        http::async_write(
            socket_,
            *resp,
            [self = shared_from_this(), resp, close](const boost::system::error_code& writeEc, std::size_t written) {
                self->onWrite(close, writeEc, written);
            }
        );
    }

    void HttpSessionBoost::onWrite(bool close, const boost::system::error_code& ec, std::size_t bytesTransferred) {
        (void)bytesTransferred;
        if (ec) {
            return;
        }

        if (close) {
            doClose();
            return;
        }

        doRead();
    }

    void HttpSessionBoost::doClose() {
        boost::system::error_code ec;
        socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);
    }

    HttpListenerBoost::HttpListenerBoost(
        boost::asio::io_context& io,
        std::shared_ptr<application::IControlRouter> router,
        std::uint16_t port
    )
        : io_(io),
          router_(std::move(router)),
          acceptor_(io_) {
        boost::asio::ip::tcp::endpoint endpoint{boost::asio::ip::tcp::v4(), port};
        acceptor_.open(endpoint.protocol(), initError_);
        if (initError_) return;
        acceptor_.set_option(boost::asio::socket_base::reuse_address(true), initError_);
        if (initError_) return;
        acceptor_.bind(endpoint, initError_);
        if (initError_) return;
        acceptor_.listen(boost::asio::socket_base::max_listen_connections, initError_);
    }

    int HttpListenerBoost::start() {
        if (started_) {
            return 0;
        }
        if (initError_) {
            return initError_.value() == 0 ? -1 : initError_.value();
        }
        started_ = true;
        doAccept();
        return 0;
    }

    int HttpListenerBoost::stop() {
        if (!started_) {
            return 0;
        }
        started_ = false;
        boost::system::error_code ec;
        acceptor_.cancel(ec);
        acceptor_.close(ec);
        return 0;
    }

    void HttpListenerBoost::doAccept() {
        if (!started_) {
            return;
        }
        acceptor_.async_accept(
            boost::beast::bind_front_handler(&HttpListenerBoost::onAccept, shared_from_this())
        );
    }

    void HttpListenerBoost::onAccept(const boost::system::error_code& ec, boost::asio::ip::tcp::socket socket) {
        if (!ec) {
            std::make_shared<HttpSessionBoost>(std::move(socket), router_)->run();
        }
        doAccept();
    }
}
