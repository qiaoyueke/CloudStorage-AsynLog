
#include "Server.hpp"
#include <thread>
using namespace std;

mylog::Util::JsonData* g_conf_data;

int main()
{
    cout<<"11111111"<<endl;
    g_conf_data = mylog::Util::JsonData::GetJsonData();

    storage::Server s;
    cout<<"222222"<<endl;

    mylog::GetLogger()->Info("service step in RunModule");
    cout<<"3333"<<endl;

    s.service();
    cout<<"4444"<<endl;

    return 0;
}