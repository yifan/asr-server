#include "RequestChunkListReader.h"

namespace apiai {

kaldi::SubVector<kaldi::BaseFloat> *RequestChunkListReader::NextChunk(kaldi::int32 samples_count, kaldi::int32 timeout_ms) {
  unique_lock<mutex> lock(chunk_queue_lock_);

  KALDI_WARN << "waiting for chunk" << std::endl;
  while (chunk_queue_.empty()) {
    chunk_queue_cond_.wait(lock);
  }

  current_chunk_ptr_.reset(chunk_queue_.front());
  chunk_queue_.pop();

  lock.unlock();

  KALDI_WARN << "got chunk " << current_chunk_ptr_->Dim() << std::endl;

  return (current_chunk_ptr_->Dim() == 0)?NULL:(new kaldi::SubVector<kaldi::BaseFloat>(current_chunk_ptr_->Data(), current_chunk_ptr_->Dim()));
}

}
