
#include "Server.hpp"
#include <thread>
using namespace std;

mylog::Util::JsonData* g_conf_data;
mylog::ThreadPool* tp;

int main()
{
    tp = new mylog::ThreadPool(mylog::Util::JsonData::GetJsonData()->thread_count_);
    g_conf_data = mylog::Util::JsonData::GetJsonData();
    storage::Server s;
    mylog::GetLogger()->Info("service start");
    mylog::GetLogger()->Error("service start");

    s.service();
    return 0;
}