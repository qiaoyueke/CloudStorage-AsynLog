#pragma once

#include "Util.hpp"
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>

namespace storage
{
    const char *congif_file = "./../storage/storage.conf";

    class Config
    {
    private:
        int server_port_;
        std::string server_ip_;
        std::string download_prefix_;  // URL路径前缀
        std::string deep_storage_dir_; // 深度存储文件的存储路径
        std::string low_storage_dir_;  // 浅度存储文件的存储路径
        std::string storage_info_;     // 已存储文件的信息
        int bundle_format_;            // 深度存储的文件后缀，由选择的压缩格式确定

        static std::mutex _mutex;
        static Config *_instance;

    private:
        Config()
        {
            if (read_config() == false)
            {
                mylog::GetLogger()->Fatal("ReadConfig failed");
                return;
            }
            mylog::GetLogger()->Info("ReadConfig complicate");
        }

    public:
        bool read_config()
        {
            std::string content ;
            storage::FileUtil f(congif_file);
            if(!f.get_file(&content))
            {
                return false;
            }
            nlohmann::json j = nlohmann::json::parse(content);
            server_port_ = j["server_port"];
            server_ip_ = j["server_ip"];
            download_prefix_ = j["download_prefix"];
            deep_storage_dir_ = j["deep_storage_dir"];
            low_storage_dir_ = j["low_storage_dir"];
            storage_info_ = j["storage_info"];
            bundle_format_ = j["bundle_format"];
            return true;
        }

        int GetServerPort()
        {
            return server_port_;
        }

        std::string GetServerIp()
        {
            return server_ip_;
        }
        std::string GetDownloadPrefix()
        {
            return download_prefix_;
        }
        std::string GetDeepStorageDir()
        {
            return deep_storage_dir_;
        }
        std::string GetLowStorageDir()
        {
            return low_storage_dir_;
        }
        std::string GetStorageInfo()
        {
            return storage_info_;
        }
        int GetBundleFormat()
        {
            return bundle_format_;
        }

    public:
        static Config *GetInstance()
        {
            if (_instance == nullptr)
            {
                _mutex.lock();
                if (_instance == nullptr)
                {
                    _instance = new Config();
                }
                _mutex.unlock();
            }
            return _instance;
        }
    };

    std::mutex Config::_mutex;
    Config *Config::_instance = nullptr;

}