#include "VideoReadyHttpResponse.h"

VideoReadyHttpResponse::VideoReadyHttpResponse() {}

char* VideoReadyHttpResponse::data() {
    return response_.data();
}

int VideoReadyHttpResponse::size() {
    return response_.size();
}
