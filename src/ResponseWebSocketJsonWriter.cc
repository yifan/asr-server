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
	lock_guard<mutex> guard(hdl_mutex_);
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

//{"status": 0, "result": {"hypotheses": [{"transcript": "see on teine lause."}], "final": true}}
void ResponseWebSocketJsonWriter::SetResult(std::vector<RecognitionResult> &data, const std::string &interrupted, int timeMarkMs) {

	std::ostringstream msg;
	msg << "{";
	msg << "\"status\":0";
	msg << ",\"result\":{\"hypotheses\":[{\"transcript\": \"";
	for (int i = 0; i < data.size(); i++) {
		msg << data.at(i).text;
		if (i != data.size() - 1) {
			msg << " ";
		}
	}
	msg << "\"}]}";
	msg << ",\"final\":true";
	msg << "}";
	std::cerr << "R " << msg.str() << std::endl;
	SendJson(msg.str(), true);
}

void ResponseWebSocketJsonWriter::SetIntermediateResult(RecognitionResult &decodedData, int timeMarkMs) {
	std::ostringstream msg;
	msg << "{";
	msg << "\"status\":0";
	msg << ",\"result\":{\"hypotheses\":[{\"transcript\": \"";
	msg << decodedData.text;
	msg << "\"}]}";
	msg << ",\"final\":false";
	msg << "}";
	std::cerr << "I " << msg.str() << std::endl;
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
