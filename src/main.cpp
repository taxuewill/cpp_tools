//
// Created by Will on 2021/8/28.
//

#include <iostream>
#include <thread>
#include "bs_tools/handler.h"
using namespace std;
using namespace bs_tools;

class MyHandler :public Handler{
public:
    MyHandler(SptrLooper looper): Handler(looper){}
    void handleMessage(SptrMessage msg){
        switch (msg->mWhat) {
            case 1:{
                cout<<"message 1"<<endl;
                break;
            }
            case 2:{
                cout<<"message 2"<<endl;
                break;
            }
            default:
                cout<<"default handle message["<<msg->mWhat<<"]"<<endl;

        }
    }
};
class MyRunnable : public Runnable{
public:
    void run(){
        cout<<"this is my runnable"<<endl;
    }
};


int main(int argc,char ** argv){
    cout<<"Hello,Cpp tools!"<<endl;

    HandlerThread handlerThread("test_handler");
    handlerThread.start();
    SptrHandler handler = std::make_shared<MyHandler>(handlerThread.getLooper());
    handler->sendEmptyMessage(1);
    handler->sendEmptyMessage(2);
    handler->sendEmptyMessageDelayed(1,1000);
    handler->post(make_shared<MyRunnable>());
    cout<<"start sleep"<<endl;
    this_thread::sleep_for(chrono::seconds(5));
    handlerThread.quit();
    cout<<"end of main"<<endl;



}