// IODecodingApp.cc

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

#include "RequestRawReader.h"
#include "ResponseJsonWriter.h"
#include "ResponseMultipartJsonWriter.h"
#include "IODecodingApp.h"
#include "QueryStringParser.h"
#include <list>
#include <string>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cctype>
#include <memory>

namespace apiai {

const std::string PARAMETER_NAME_NBEST = "nbest";
const std::string PARAMETER_NAME_INTERMEDIATE = "intermediate";
const std::string PARAMETER_NAME_END_OF_SPEECH = "endofspeech";
const std::string PARAMETER_MULTIPART = "multipart";

class ResponseParams {
public:
	bool multipart;

	static bool default_multipart;
	static bool default_endofspeech;

	ResponseParams() : multipart(default_multipart) {};
};

// Multipart option default value
bool ResponseParams::default_multipart = false;

// End-of-speech detection option default value
bool ResponseParams::default_endofspeech = true;

bool to_bool(std::string &str) {
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    std::istringstream is(str);
    bool b;
    is >> std::boolalpha >> b;
    return b;
}

bool to_bool(const char *chars) {
	std::string str(chars);
    return to_bool(str);
}

/*
void apply_request_parameters(FCGX_Request &request, RequestRawReader &reader, ResponseParams &params) {
	char *queryString = FCGX_GetParam("QUERY_STRING", request.envp);
	if (queryString) {
		QueryStringParser queryStringParser(queryString);
		std::string name, value;
		while (queryStringParser.Next(&name, &value)) {
			if (PARAMETER_NAME_NBEST == name) {
				reader.BestCount(atoi(value.data()));
				KALDI_VLOG(1) << "Setting n-best: " << reader.BestCount();
			} else if (PARAMETER_NAME_INTERMEDIATE == name) {
				reader.IntermediateIntervalMillisec(atoi(value.data()));
				KALDI_VLOG(1) << "Setting intermediate interval: " << reader.IntermediateIntervalMillisec() << " ms";
			} else if (PARAMETER_NAME_END_OF_SPEECH == name) {
				reader.DoEndpointing(to_bool(value.data()));
				KALDI_VLOG(1) << "Setting end-of-speech: " << (reader.DoEndpointing() ? "enabled" : "disabled");
			} else if (PARAMETER_MULTIPART == name) {
				params.multipart = to_bool(value.data());
				KALDI_VLOG(1) << "Setting multipart: " << (params.multipart ? "enabled" : "disabled");
			} else {
				KALDI_VLOG(1) << "Skipping unknown parameter \"" << name << "\"";
			}
		}
	}
}
*/

void IODecodingApp::RegisterOptions(kaldi::OptionsItf &po) {
    po.Register("fcgi-socket", &fcgi_socket_path_, "FastCGI connection string, if undefined then stdin and stdout will be used");
    po.Register("fcgi-socket.backlog", &fcgi_socket_backlog_, "FastCGI socket backlog size.");
    po.Register("fcgi-threads-number", &fcgi_threads_number_, "Number of FastCGI working threads");
    po.Register("fcgi-multipart", &ResponseParams::default_multipart, "Enable or disable multipart responses by default");
    po.Register("fcgi-endofspeech", &ResponseParams::default_endofspeech, "Enable or disable end-of-speech detection by default");
}

void *IODecodingApp::RunChildThread(void *arg) {
	IODecodingApp *app = (IODecodingApp*)arg;
	Decoder *decoder = app->decoder_.Clone();
	app->ProcessingRoutine(*decoder);
	delete decoder;
	return NULL;
}

void IODecodingApp::ProcessingRoutine(Decoder &decoder) {
    if (socket_id_ < 0) {
	KALDI_WARN << "Socket not opened";
	return;
    }

	try {
		RequestRawReader reader(&std::cin);

		reader.DoEndpointing(ResponseParams::default_endofspeech);

		ResponseParams params;
		//apply_request_parameters(request, reader, params);

		std::auto_ptr<ResponseJsonWriter> writer_ptr;
		if (params.multipart) {
			writer_ptr.reset(new ResponseMultipartJsonWriter(&std::cout));
		} else {
			writer_ptr.reset(new ResponseJsonWriter(&std::cout));
		}

		//std::cout << "Content-type: "<< writer_ptr.get()->GetContentType() <<"\r\n\r\n";

		decoder.Decode(reader, *(writer_ptr.get()));
	} catch (std::exception &e) {
		KALDI_LOG << "Fatal exception: " << e.what();
	}

}

int IODecodingApp::Run(int argc, char **argv) {

	if (running_) {
		KALDI_WARN << "Application already running";
		return 1;
	}
	running_ = true;

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

		KALDI_VLOG(1) << "Single thread running";
		ProcessingRoutine(decoder_);

	running_ = false;
	return 0;
}
} /* namespace apiai */
