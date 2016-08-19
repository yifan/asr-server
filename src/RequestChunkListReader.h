// RequestChunkListReader.h

// by Yifan Zhang (yzhang@qf.org.qa)
// Copyright (C) 2016, Qatar Computing Research Institute, HBKU

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

#ifndef DECODER_CHUNKREQUEST_H_
#define DECODER_CHUNKREQUEST_H_

#include "Request.h"

#include <queue>
#include <memory>
#include <websocketpp/common/thread.hpp>

using websocketpp::lib::thread;
using websocketpp::lib::mutex;
using websocketpp::lib::lock_guard;
using websocketpp::lib::unique_lock;
using websocketpp::lib::condition_variable;


namespace apiai {

/**
 * Request data holding interface
 */
class RequestChunkListReader : public Request {
public:
  RequestChunkListReader() {
    frequency_ = 16000;
    bytes_per_sample_ = 16 / 8;
    channels_ = 1;
    channel_index_ = 0;
  }

  void feed(const std::vector<float> &data) {
    kaldi::Vector<kaldi::BaseFloat> *chunk = new kaldi::Vector<kaldi::BaseFloat>(data.size());
    for (int i = 0; i < data.size(); i ++) {
      (*chunk)(i) = data[i];
    }
    {
      std::lock_guard<mutex> guard(chunk_queue_lock_);
      chunk_queue_.push(chunk);
    }
  }

	virtual ~RequestChunkListReader() {};

	/** Get number of samples per second of audio data */
	virtual kaldi::int32 Frequency(void) const {
    return frequency_; 
  }

	/** Get max number of expected result variants */
	virtual kaldi::int32 BestCount(void) const = 0;
	/** Get milliseconds interval between intermediate results.
	 *  If non-positive given then no intermediate results would be calculated */
	virtual kaldi::int32 IntermediateIntervalMillisec(void) const {
    return intermediateMillisecondsInterval_;
  }

	/** Get end-of-speech points detection flag. */
	virtual bool DoEndpointing(void) const = 0;

	/**
	 * Get next chunk of audio data samples.
	 * Max number of samples specified by samples_count value
	 */
	virtual kaldi::SubVector<kaldi::BaseFloat> *NextChunk(kaldi::int32 samples_count) {
    return this->NextChunk(samples_count, 0);
  }
	/**
	 * Get next chunk of audio data samples.
	 * Max number of samples specified by samples_count value.
	 * Read timeout specified by timeout_ms.
	 */
	virtual kaldi::SubVector<kaldi::BaseFloat> *NextChunk(kaldi::int32 samples_count, kaldi::int32 timeout_ms);

private:
  kaldi::int32 frequency_;
  kaldi::int32 bytes_per_sample_;
  kaldi::int32 channels_;
  kaldi::int32 channel_index_;

  kaldi::int32 intermediateMillisecondsInterval_;
  std::unique_ptr<kaldi::Vector<kaldi::BaseFloat>> current_chunk_ptr_;
  std::queue<kaldi::Vector<kaldi::BaseFloat>*> chunk_queue_;
  mutex chunk_queue_lock_;
  condition_variable chunk_queue_cond_;
};

} /* namespace apiai */

#endif /* SRC_REQUEST_H_ */
