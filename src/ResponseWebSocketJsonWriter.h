// ResponseJsonWriter.h

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//  http://www.apache.org/licenses/LICENSE-2.0
//
// THIS CODE IS PROVIDED *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION ANY IMPLIED
// WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR PURPOSE,
// MERCHANTABLITY OR NON-INFRINGEMENT.
// See the Apache 2 License for the specific language governing permissions and
// limitations under the License.

#ifndef RESPONSEWSJSONWRITER_H_
#define RESPONSEWSJSONWRITER_H_
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include "Response.h"
#include <sstream>
typedef websocketpp::server<websocketpp::config::asio> server;

using websocketpp::connection_hdl;

namespace apiai {

/**
 * Writes recognition data to output stream as JSON serialized objects
 */
class ResponseWebSocketJsonWriter : public Response {
public:
	ResponseWebSocketJsonWriter(server *serve, connection_hdl hdl) : hdl_(hdl), server_(serve) {}
	virtual ~ResponseWebSocketJsonWriter() {};

	virtual const std::string &GetContentType() { return MIME_APPLICATION_JAVA; }

	virtual void SetResult(std::vector<RecognitionResult> &data, int timeMarkMs);
	virtual void SetResult(std::vector<RecognitionResult> &data, const std::string &interrupted, int timeMarkMs);
	virtual void SetIntermediateResult(RecognitionResult &decodedData, int timeMarkMs);
	virtual void SetError(const std::string &message);
protected:

	virtual void SendJson(std::string json, bool final);
private:
	void Write(std::ostringstream &outss, RecognitionResult &data);

	static const std::string MIME_APPLICATION_JAVA;
	connection_hdl hdl_;
	server *server_;
};

} /* namespace apiai */

#endif /* RESPONSEWSJSONWRITER_H_ */
