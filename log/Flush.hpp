#pragma once

#include<memory>

namespace log{
//Flush的基类
class LogFlush
{
    public:
    using ptr = std::shared_ptr<LogFlush>;
};

}