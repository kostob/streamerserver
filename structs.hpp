/* 
 * File:   structs.hpp
 * Author: tobias
 *
 * Created on 29. Oktober 2013, 10:25
 */

#ifndef STRUCTS_HPP
#define	STRUCTS_HPP

#ifdef __cplusplus
extern "C" {
#endif
#include <libavformat/avformat.h>
#ifdef __cplusplus
}
#endif

struct InputStream {
  AVFormatContext *s;
  int audio_stream;
  int video_stream;
  int64_t last_ts;
  int frames_since_global_headers;
};

struct InputDescription {
    InputStream *s;
    int id;
    int interframe;
    uint64_t start_time;
    uint64_t first_ts;
};



#endif	/* STRUCTS_HPP */

