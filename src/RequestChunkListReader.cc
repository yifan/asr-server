#include "RequestChunkListReader.h"

namespace apiai {

kaldi::SubVector<kaldi::BaseFloat> *RequestChunkListReader::NextChunk(kaldi::int32 samples_count, kaldi::int32 timeout_ms) {
  unique_lock<mutex> lock(chunk_queue_lock_);

  while (chunk_queue_.empty()) {
    chunk_queue_cond_.wait(lock);
  }

  current_chunk_ptr_.reset(chunk_queue_.front());
  chunk_queue_.pop();

  lock.unlock();

  return new kaldi::SubVector<kaldi::BaseFloat>(current_chunk_ptr_->Data(), current_chunk_ptr_->Dim());
}

}
