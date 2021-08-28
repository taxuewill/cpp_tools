//
// Created by Will on 2021/8/28.
//

#include <iostream>
#include <thread>
#include "bs_tools/handler.h"
using namespace std;
using namespace bs_tools;

int main(int argc,char ** argv){
    cout<<"Hello,Cpp tools!"<<endl;
    time_t now = getCurrentTimeStamp();
    cout<<now<<endl;
    this_thread::sleep_for(chrono::seconds(1));
    time_t endtime = getCurrentTimeStamp();
    cout<<endtime<<endl;
    cout<<(endtime-now)<<endl;
}