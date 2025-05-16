#pragma once

#include<string>


namespace mylog
{
    class LogLevel
    {
    public:
        enum class value
        {
            DEBUG,
            INFO,
            WARN,
            ERROR,
            FATAL
        };

        static const char *level_to_string(const value level)
        {
            switch (level)
            {
            case value::DEBUG:
                return "DEBUG";
            case value::INFO:
                return "INFO";
            case value::WARN:
                return "WARN";
            case value::ERROR:
                return "ERROR";
            case value::FATAL:
                return "FATAL";
            default:
                return "UNKNOW";
            }
            return "UNKNOW";
        }
    };
}