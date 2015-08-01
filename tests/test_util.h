/*
** Common functionality for tests.
**/

#ifndef UPB_TEST_UTIL_H_
#define UPB_TEST_UTIL_H_

#include <stdio.h>
#include <math.h>
#include "tests/upb_test.h"
#include "upb/env.h"
#include "upb/sink.h"

upb::BufferHandle global_handle;

// A convenience class for parser tests.  Provides some useful features:
//
//   - can support multiple calls to parse, to test the parser's handling
//     of buffer seams.
//
//   - can output verbose output about each parse call when requested, for
//     ease of debugging.
//
//   - can pass NULL for skipped regions of the input if requested.
//
//   - allocates and passes a separate buffer for each parsed region, to
//     ensure that the parser is not erroneously overreading its buffer.
class VerboseParserEnvironment {
 public:
  // Pass verbose=true to print detailed diagnostics to stderr.
  VerboseParserEnvironment(bool verbose) : verbose_(verbose) {
    env_.ReportErrorsTo(&status_);
  }

  void Reset(const char *buf, size_t len, bool may_skip) {
    buf_ = buf;
    len_ = len;
    ofs_ = 0;
    skip_until_ = may_skip ? 0 : -1;
    skipped_with_null_ = false;
    status_.Clear();
  }

  // The user should call a series of:
  //
  // Reset(buf, len, may_skip);
  // Start()
  // ParseBuffer(X);
  // ParseBuffer(Y);
  // // Repeat ParseBuffer as desired, but last call should pass -1.
  // ParseBuffer(-1);
  // End();


  bool Start() {
    if (verbose_) {
      fprintf(stderr, "Calling start()\n");
    }
    return sink_->Start(len_, &subc_);
  }

  bool End() {
    if (verbose_) {
      fprintf(stderr, "Calling end()\n");
    }
    return sink_->End();
  }

  bool ParseBuffer(int bytes) {
    if (bytes < 0) {
      bytes = len_ - ofs_;
    }

    ASSERT((size_t)bytes <= (len_ - ofs_));

    // Copy buffer into a separate, temporary buffer.
    // This is necessary to verify that the parser is not erroneously
    // reading outside the specified bounds.
    char *buf2 = NULL;

    if ((int)(ofs_ + bytes) <= skip_until_) {
      skipped_with_null_ = true;
    } else {
      buf2 = (char*)malloc(bytes);
      assert(buf2);
      memcpy(buf2, buf_ + ofs_, bytes);
    }

    if (buf2 == NULL && bytes == 0) {
      // Decoders dont' support buf=NULL, bytes=0.
      return true;
    }

    if (verbose_) {
      fprintf(stderr, "Calling parse(%u) for bytes %u-%u of the input\n",
              (unsigned)bytes, (unsigned)ofs_, (unsigned)(ofs_ + bytes));
    }

    int parsed = sink_->PutBuffer(subc_, buf2, bytes, &global_handle);
    free(buf2);

    if (verbose_) {
      if (parsed == bytes) {
        fprintf(stderr,
                "parse(%u) = %u, complete byte count indicates success\n",
                (unsigned)bytes, (unsigned)bytes);
      } else if (parsed > bytes) {
        fprintf(stderr,
                "parse(%u) = %u, long byte count indicates success and skip "
                "of the next %u bytes\n",
                (unsigned)bytes, (unsigned)parsed, (unsigned)(parsed - bytes));
      } else {
        fprintf(stderr,
                "parse(%u) = %u, short byte count indicates failure; "
                "last %u bytes were not consumed\n",
                (unsigned)bytes, (unsigned)parsed, (unsigned)(bytes - parsed));
      }
    }

    if (status_.ok() != (parsed >= bytes)) {
      if (status_.ok()) {
        fprintf(stderr,
                "Error: decode function returned short byte count but set no "
                "error status\n");
      } else {
        fprintf(stderr,
                "Error: decode function returned complete byte count but set "
                "error status\n");
      }
      fprintf(stderr, "Status: %s, parsed=%u, bytes=%u\n",
              status_.error_message(), (unsigned)parsed, (unsigned)bytes);
      ASSERT(false);
    }

    if (!status_.ok())
      return false;

    if (parsed > bytes && skip_until_ >= 0) {
      skip_until_ = ofs_ + parsed;
    }

    ofs_ += UPB_MIN(parsed, bytes);

    return true;
  }

  void ResetBytesSink(upb::BytesSink* sink) {
    sink_ = sink;
  }

  const upb::Status& status() { return status_; }

  size_t ofs() { return ofs_; }
  upb::Environment* env() { return &env_; }

  bool SkippedWithNull() { return skipped_with_null_; }

 private:
  upb::Environment env_;
  upb::Status status_;
  upb::BytesSink* sink_;
  const char* buf_;
  size_t len_;
  bool verbose_;
  size_t ofs_;
  void *subc_;

  // When our parse call returns a value greater than the number of bytes
  // we passed in, the decoder is indicating to us that the next N bytes
  // in the stream are not needed and can be skipped.  The user is allowed
  // to pass a NULL buffer for those N bytes.
  //
  // skip_until_ is initially set to 0 if we should do this NULL-buffer
  // skipping or -1 if we should not.  If we are open to doing NULL-buffer
  // skipping and we get an opportunity to do it, we set skip_until to the
  // stream offset where we can skip until.  The user can then test whether
  // this happened by testing SkippedWithNull().
  int skip_until_;
  bool skipped_with_null_;
};

#endif
