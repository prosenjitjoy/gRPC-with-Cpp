#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>

#include <chrono>
#include <print>

#include "clientguide.grpc.pb.h"

int64_t now() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}

class ClientGuideService : public clientguide::ClientGuideService::Service {
  grpc::Status UnaryCall(grpc::ServerContext* context,
                         const clientguide::Request* request,
                         clientguide::Response* response) override {
    std::println("Server (UnaryCall): time: {}, num: {}", request->time(),
                 request->num());

    if (request->time() > now()) {
      return {grpc::StatusCode::INVALID_ARGUMENT,
              "Request time is in the future!"};
    }

    response->set_num(request->num() + 1);
    response->set_time(now());

    return grpc::Status::OK;
  }

  grpc::Status ServerStreaming(
      grpc::ServerContext* context, const clientguide::Request* request,
      grpc::ServerWriter<clientguide::Response>* writer) override {
    std::println("Server (ServerStreaming): time: {}, num: {}", request->time(),
                 request->num());

    if (request->time() > now()) {
      return {grpc::StatusCode::INVALID_ARGUMENT,
              "Request time is in the future!"};
    }

    clientguide::Response response;
    for (int32_t i = 0; i < request->num(); i++) {
      response.set_num(i);
      response.set_time(now());
      writer->Write(response);
    }

    return grpc::Status::OK;
  }

  grpc::Status ClientStreaming(grpc::ServerContext* context,
                               grpc::ServerReader<clientguide::Request>* reader,
                               clientguide::Response* response) override {
    clientguide::Request request;
    int32_t count = 0;
    while (reader->Read(&request)) {
      std::println("Server (ClientStreaming): time: {}, num: {}",
                   request.time(), request.num());
      if (request.time() > now()) {
        return {grpc::StatusCode::INVALID_ARGUMENT,
                "Request time is in the future!"};
      }
      count++;
    }

    response->set_num(count);
    response->set_time(now());

    return grpc::Status::OK;
  }

  grpc::Status BidirectionalStreaming(
      grpc::ServerContext* context,
      grpc::ServerReaderWriter<clientguide::Response, clientguide::Request>*
          stream) override {
    clientguide::Request request;
    clientguide::Response response;

    while (stream->Read(&request)) {
      std::println("Server (BidirectionalStreaming): time: {}, num: {}",
                   request.time(), request.num());

      if (request.time() > now()) {
        return {grpc::StatusCode::INVALID_ARGUMENT,
                "Request time is in the future!"};
      }

      response.set_num(request.num());
      response.set_time(now());
      if (!stream->Write(response)) {
        return grpc::Status::CANCELLED;
      }
    }

    return grpc::Status::OK;
  };
};

int main() {
  std::unique_ptr<grpc::Server> server;
  ClientGuideService service;

  grpc::ServerBuilder builder;
  builder.AddListeningPort("localhost:5000", grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  server = builder.BuildAndStart();

  std::println("Server listening on: localhost:5000");
  server->Wait();
}
