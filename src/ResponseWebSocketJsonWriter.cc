// ResponseWebSocketJsonWriter.cc

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

#include "ResponseWebSocketJsonWriter.h"

namespace apiai {

const std::string ResponseWebSocketJsonWriter::MIME_APPLICATION_JAVA = "application/json";

void ResponseWebSocketJsonWriter::SendJson(std::string json, bool final) {
	server_->send(hdl_, json, websocketpp::frame::opcode::text);
}

void ResponseWebSocketJsonWriter::Write(std::ostringstream &outss, RecognitionResult &data) {
  outss << "{"
  	<< "\"confidence\":" << data.confidence << ","
  	<< "\"text\":\"" << data.text << "\""
  	<< "}";
}

void ResponseWebSocketJsonWriter::SetResult(std::vector<RecognitionResult> &data, int timeMarkMs) {
	SetResult(data, NOT_INTERRUPTED, timeMarkMs);
}

void ResponseWebSocketJsonWriter::SetResult(std::vector<RecognitionResult> &data, const std::string &interrupted, int timeMarkMs) {

	std::ostringstream msg;
	msg << "{";
	msg << "\"status\":\"ok\"";
	msg << ",\"data\":[";
	for (int i = 0; i < data.size(); i++) {
		if (i) {
			msg << ",";
		}
		Write(msg, data.at(i));
	}
	msg << "]";
	if (interrupted.size() > 0) {
		msg << ",\"interrupted\":\"" << interrupted << "\"";
		if (timeMarkMs > 0) {
		    msg << ",\"time\":" << timeMarkMs;
		}
	}
	msg << "}";
	SendJson(msg.str(), true);
}

void ResponseWebSocketJsonWriter::SetIntermediateResult(RecognitionResult &decodedData, int timeMarkMs) {
	std::ostringstream msg;
	msg << "{";
	msg << "\"status\":\"intermediate\"";
	msg << ",\"data\":[";
	Write(msg, decodedData);
	msg << "]}";
	SendJson(msg.str(), false);
}

void ResponseWebSocketJsonWriter::SetError(const std::string &message) {
	std::ostringstream msg;
    msg << "{";
    msg << "\"status\":\"error\"";
    msg << ",\"data\":[{\"text\":\""<< message << "\"}]";
    msg << "}";
    SendJson(msg.str(), true);
}

} /* namespace apiai */
