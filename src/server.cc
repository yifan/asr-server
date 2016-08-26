#include <iostream>

#include "Decoder.h"
#include "RequestChunkListReader.h"
#include "Nnet3LatgenFasterDecoder.h"

#include <boost/algorithm/string/predicate.hpp>

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <websocketpp/common/thread.hpp>
typedef websocketpp::server<websocketpp::config::asio> server;
#include "ResponseWebSocketJsonWriter.h"

using websocketpp::connection_hdl;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

using websocketpp::lib::thread;
using websocketpp::lib::mutex;
using websocketpp::lib::lock_guard;
using websocketpp::lib::unique_lock;
using websocketpp::lib::condition_variable;

namespace apiai {

class WebSocketServer {
  typedef server::connection_ptr connection_ptr;

 public:
  WebSocketServer(Decoder &decoder) : port_(8888), decoder_(decoder),
    request(new RequestChunkListReader()) {

    response.reset( new ResponseWebSocketJsonWriter(&server_));

    server_.init_asio();

    server_.set_open_handler(bind(&WebSocketServer::onOpen, this, ::_1));
    server_.set_close_handler(bind(&WebSocketServer::onClose, this, ::_1));
    server_.set_message_handler(bind(&WebSocketServer::onMessage, this, ::_1, ::_2));
  }

  int prepare(int argc, char **argv) {
    // Predefined configuration args
    const char *extra_args[] = {
      "--feature-type=mfcc",
      "--mfcc-config=mfcc.conf",
      "--frame-subsampling-factor=3",
      "--max-active=2000",
      "--beam=15.0",
      "--lattice-beam=6.0",
      "--acoustic-scale=1.0",
      "--endpoint.silence-phones=1",
      "--endpoint.rule1.min-trailing-silence=0.5",
      "--endpoint.rule2.min-trailing-silence=0.15",
      "--endpoint.rule3.min-trailing-silence=0.1",
    };

    kaldi::ParseOptions po(usage_.data());
    RegisterOptions(po);
    decoder_.RegisterOptions(po);

    std::vector<const char*> args;
    args.push_back(argv[0]);
    args.insert(args.end(), extra_args, extra_args + sizeof(extra_args) / sizeof(extra_args[0]));
    args.insert(args.end(), argv + 1, argv + argc);
    po.Read(args.size(), args.data());

    if (!decoder_.Initialize(po)) {
      po.PrintUsage();
      running_ = false;
      return 1;
    }
    return 0;
  }

  void run() {
    server_.listen(port_);
    server_.start_accept();

    try {
      server_.run();
    } catch (const std::exception &e) {
      std::cout << e.what() << std::endl;
    }
  }

  void decoderThread() {
    KALDI_WARN << "Decoder entered" << std::endl;
    try {
      while (decoder_.Decode(*(request.get()), *(response.get())) == Response::INTERRUPTED_END_OF_SPEECH) {
      };
    } catch (std::exception &e) {
      KALDI_LOG << "Fatal exception: " << e.what();
    }
    KALDI_WARN << "Decoder exited" << std::endl;
  }

  void onOpen(connection_hdl hdl) {
    connection_ptr con = server_.get_con_from_hdl(hdl);
    std::string filename = con->get_resource();
    std::cerr << "OPEN from connection " << filename << std::endl;
    if (filename == "/client/ws/status") {
      KALDI_WARN << "OPEN /client/ws/status from connection " << filename << std::endl;
      lock_guard<mutex> guard(listener_lock_);
      // status_listeners_.insert(hdl);
    } else {
      response->SetHdl(hdl);
      KALDI_WARN << "OPEN /client/ws/speech from connection " << filename << std::endl;
    }

  }

  void onClose(connection_hdl hdl) {
    connection_ptr con = server_.get_con_from_hdl(hdl);
    std::string filename = con->get_resource();
    std::cerr << "CLOSE from connection " << filename << std::endl;
    if (filename == "/client/ws/status") {
      std::cerr << "CLOSE /client/ws/status from connection " << filename << std::endl;
      lock_guard<mutex> guard(listener_lock_);
      // status_listeners_.erase(hdl);
    } else if (filename == "/client/ws/speech") {
    }
  }

  void onMessage(connection_hdl hdl, server::message_ptr msg) {
    {
      if (msg->get_opcode() == websocketpp::frame::opcode::text) {
        std::cout << "MESSAGE: " << msg->get_payload() << std::endl;
        if (msg->get_payload() == "EOF") {
          request->feed(std::vector<float>(0));
        }
      } else {
        int size = msg->get_payload().size() / 2;
        const short *raw = reinterpret_cast<const short *>(msg->get_payload().c_str());
        std::vector<float> frame(size);

        for (int i = 0; i < size; i++)
          frame[i] = raw[i];
        request->feed(frame);
      }
    }
  }

  void notifyListeners(const std::string &message) {

  }

 private:
  void RegisterOptions(kaldi::OptionsItf &po) {
    po.Register("port", &port_, "port to serve websocket");
  }

  std::string usage_;

  server server_;
  uint32 port_;
  Decoder &decoder_;
  std::auto_ptr<RequestChunkListReader> request;
  std::auto_ptr<ResponseWebSocketJsonWriter> response;

  mutex listener_lock_;

  bool running_;
};

void runDecoder(WebSocketServer *s) {
  s->decoderThread();
}

}// namepspace apiai

int main(int argc, char **argv) {
  using namespace apiai;
  Nnet3LatgenFasterDecoder decoder;
  WebSocketServer webSocketServer(decoder);

  webSocketServer.prepare(argc, argv);

  thread t(runDecoder, &webSocketServer);
  webSocketServer.run();

  return 0;
}
