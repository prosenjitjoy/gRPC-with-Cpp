#include <QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QUrl>
#include <QtGrpc/QGrpcHttp2Channel>
#include <limits>
#include <memory>

#include "clientguide.qpb.h"
#include "clientguide_client.grpc.qpb.h"

class ClientGuide : public QObject {
 public:
  explicit ClientGuide(std::shared_ptr<QGrpcHttp2Channel> channel) {
    m_client.attachChannel(std::move(channel));
  }

  static clientguide::Request createRequest(int32_t num, bool fail = false) {
    clientguide::Request request;
    request.setNum(num);
    request.setTime(fail ? std::numeric_limits<int64_t>::max()
                         : QDateTime::currentMSecsSinceEpoch());
    return request;
  }

  void unaryCall(const clientguide::Request &request) {
    std::unique_ptr<QGrpcCallReply> reply = m_client.UnaryCall(request);
    const auto *replyPtr = reply.get();

    QObject::connect(
        replyPtr, &QGrpcCallReply::finished, replyPtr,
        [reply = std::move(reply)](const QGrpcStatus &status) {
          if (status.isOk()) {
            if (const auto response = reply->read<clientguide::Response>()) {
              qDebug() << "Client (UnaryCall) finished, received: (time: "
                       << response->time() << ", num: " << response->num()
                       << ")";
            } else {
              qDebug() << "Client (UnaryCall) deserialization failed";
            }
          } else {
            qDebug() << "Client (UnaryCall) failed: " << status.message();
          }
        },
        Qt::SingleShotConnection);
  }

  void serverStreaming(const clientguide::Request &initialRequest) {
    std::unique_ptr<QGrpcServerStream> stream =
        m_client.ServerStreaming(initialRequest);
    const auto *streamPtr = stream.get();

    QObject::connect(
        streamPtr, &QGrpcServerStream::finished, streamPtr,
        [stream = std::move(stream)](const QGrpcStatus &status) {
          if (status.isOk()) {
            qDebug() << "Client (ServerStreaming) finished";
          } else {
            qDebug() << "Client (ServerStreaming) failed";
          }
        },
        Qt::SingleShotConnection);

    QObject::connect(
        streamPtr, &QGrpcServerStream::messageReceived, streamPtr, [streamPtr] {
          if (const auto response = streamPtr->read<clientguide::Response>()) {
            qDebug() << "Client (ServerStream) received: (time: "
                     << response->time() << ", num: " << response->num() << ")";
          } else {
            qDebug() << "Client (ServerStream) deserialization failed";
          }
        });
  }

  void clientStreaming(const clientguide::Request &initialRequest) {
    m_clientStream = m_client.ClientStreaming(initialRequest);
    for (int32_t i = 1; i < 3; i++) {
      m_clientStream->writeMessage(createRequest(initialRequest.num() + i));
    }
    m_clientStream->writesDone();

    QObject::connect(
        m_clientStream.get(), &QGrpcClientStream::finished,
        m_clientStream.get(), [this](const QGrpcStatus &status) {
          if (status.isOk()) {
            if (const auto response =
                    m_clientStream->read<clientguide::Response>()) {
              qDebug() << "Client (ClientStreaming) finished, received: (time: "
                       << response->time() << ", num: " << response->num()
                       << ")";
            }
            m_clientStream.reset();
          } else {
            qDebug() << "Client (ClientStreaming) failed: " << status.message();
            qDebug() << "Restarting the client stream";
            clientStreaming(createRequest(0));
          }
        });
  }

  void bidirectionalStreaming(const clientguide::Request &initialRequest) {
    m_bidiStream = m_client.BidirectionalStreaming(initialRequest);
    QObject::connect(m_bidiStream.get(), &QGrpcBidiStream::finished, this,
                     &ClientGuide::bidiFinished);
    QObject::connect(m_bidiStream.get(), &QGrpcBidiStream::messageReceived,
                     this, &ClientGuide::bidiMessageReceived);
  }

 private slots:
  void bidiFinished(const QGrpcStatus &status) {
    if (status.isOk()) {
      qDebug() << "Client (BidirectionalStreaming) finished";
    } else {
      qDebug() << "Client (BidirectionalStreaming) failed: "
               << status.message();
    }
    m_bidiStream.reset();
  }

  void bidiMessageReceived() {
    if (m_bidiStream->read(&m_bidiResponse)) {
      qDebug() << "Client (BidirectionalStreaming) received: (time: "
               << m_bidiResponse.time() << ", num: " << m_bidiResponse.num()
               << ")";
      if (m_bidiResponse.num() > 0) {
        m_bidiStream->writeMessage(createRequest(m_bidiResponse.num() - 1));
        return;
      }
    } else {
      qDebug() << "Client (BidirectionalStreaming) deserialization failed";
    }
    m_bidiStream->writesDone();
  }

 private:
  clientguide::ClientGuideService::Client m_client;
  std::unique_ptr<QGrpcClientStream> m_clientStream;
  std::unique_ptr<QGrpcBidiStream> m_bidiStream;
  clientguide::Response m_bidiResponse;
};

int main(int argc, char *argv[]) {
  QCoreApplication a(argc, argv);

  std::shared_ptr<QGrpcHttp2Channel> channel =
      std::make_shared<QGrpcHttp2Channel>(QUrl("http://localhost:5000"));

  ClientGuide clientGuide(channel);
  clientGuide.unaryCall(ClientGuide::createRequest(1));
  clientGuide.serverStreaming(ClientGuide::createRequest(3));
  clientGuide.clientStreaming(ClientGuide::createRequest(0, true));
  clientGuide.bidirectionalStreaming(ClientGuide::createRequest(3));

  return a.exec();
}
