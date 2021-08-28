//
// Created by Will on 2021/8/28.
//

#include "bs_tools/handler.h"

namespace bs_tools{

    using namespace std;
    using namespace chrono;

    Message::Message() :_when(getCurrentTimeStamp()),_what(-1),_sptr_runnable(nullptr),_next(nullptr) {}

    time_t getCurrentTimeStamp(){
        time_point<system_clock,milliseconds> tp = time_point_cast<milliseconds>(system_clock::now());
        auto tmp = duration_cast<milliseconds>(tp.time_since_epoch());
        time_t timestamp = tmp.count();
        return timestamp;
    }

}

