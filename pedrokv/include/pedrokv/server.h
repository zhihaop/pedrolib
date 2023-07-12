#ifndef PEDROKV_KV_SERVER_H
#define PEDROKV_KV_SERVER_H

#include "pedrokv/codec/server_codec.h"
#include "pedrokv/defines.h"
#include "pedrokv/logger/logger.h"
#include "pedrokv/options.h"

#include <memory>
#include <pedrodb/db.h>
#include <pedronet/tcp_server.h>
#include <utility>
namespace pedrokv {

class Server : nonmovable,
               noncopyable,
               public std::enable_shared_from_this<Server> {
  pedronet::TcpServer server_;
  pedronet::InetAddress address_;
  ServerOptions options_;

  std::shared_ptr<pedrodb::DB> db_;
  ServerCodec codec_;

  Response ProcessRequest(const Request &request) {
    Response response;
    response.id = request.id;
    
    pedrodb::Status status;
    switch (request.type) {
    case Request::Type::kGet: {
      status = db_->Get({}, request.key, &response.data);
      break;
    }
    case Request::Type::kDelete: {
      status = db_->Delete({}, request.key);
      break;
    }
    case Request::Type::kSet: {
      status = db_->Put({}, request.key, request.value);
      break;
    }
    default: {
      PEDROKV_WARN("invalid request receive");
      break;
    }
    }

    if (status != pedrodb::Status::kOk) {
      response.type = Response::Type::kError;
      response.data = fmt::format("err: {}", status);
    } else {
      response.type = Response::Type::kOk;
    }
    return response;
  }

public:
  Server(pedronet::InetAddress address, ServerOptions options)
      : address_(std::move(address)), options_(std::move(options)) {
    server_.SetGroup(options_.boss_group, options_.worker_group);

    auto stat = pedrodb::DB::Open(options_.db_options, options_.db_path, &db_);
    if (stat != pedrodb::Status::kOk) {
      PEDROKV_FATAL("failed to open db {}", options_.db_path);
    }

    codec_.OnMessage([this](const auto &conn, const Request &request) {
      Response response = ProcessRequest(request);
      ServerCodec::SendResponse(conn, response);
    });

    codec_.OnConnect([](const pedronet::TcpConnectionPtr &conn) {
      PEDROKV_INFO("connect to client {}", *conn);
    });

    codec_.OnClose([](const pedronet::TcpConnectionPtr &conn) {
      PEDROKV_INFO("disconnect to client {}", *conn);
    });

    server_.OnError([](const pedronet::TcpConnectionPtr &conn, Error err) {
      PEDROKV_ERROR("client {} error: {}", *conn, err);
    });

    server_.OnConnect(codec_.GetOnConnect());
    server_.OnClose(codec_.GetOnClose());
    server_.OnMessage(codec_.GetOnMessage());
  }

  void Bind() {
    server_.Bind(address_);
    PEDROKV_INFO("server bind success: {}", address_);
  }

  void Start() {
    server_.Start();
    PEDROKV_INFO("server start");
  }
};

} // namespace pedrokv

#endif // PEDROKV_KV_SERVER_H
