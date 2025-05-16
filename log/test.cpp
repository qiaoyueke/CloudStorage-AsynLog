#include "Util.hpp"
#include "Manager.hpp"
#include "MyLog.hpp"

mylog::Util::JsonData *g_conf_data;

void test()
{
    std::cout << "test start" << std::endl;
    int cur_size = 0;
    int cnt = 1;
    while (cur_size++ < 2)
    {
        mylog::GetLogger("asynclogger")->Info("测试日志-%d", cnt++);
        mylog::GetLogger("asynclogger")->Warn("测试日志-%d", cnt++);
        mylog::GetLogger("asynclogger")->Debug("测试日志-%d", cnt++);
        mylog::GetLogger("asynclogger")->Error("测试日志-%d", cnt++);
        mylog::GetLogger("asynclogger")->Fatal("测试日志-%d", cnt++);
    }
}

int main()
{
    g_conf_data = mylog::Util::JsonData::GetJsonData();
    mylog::LoggerBuilder *LGB = new mylog::LoggerBuilder();
    LGB->SetLoggerName("asynclogger");
    LGB->AddFlush<mylog::FileFlush>("./log.log");
    LGB->AddFlush<mylog::RollFileFlush>("./log", 1024 * 1024);
    mylog::LoggerManager::GetInstance().AddLogger(LGB->Build());
    test();
    return 0;
}
