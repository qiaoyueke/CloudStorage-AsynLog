#pragma once
#include <ctime>
#include <unordered_map>
#include <string>
#include <pthread.h>

#include "Util.hpp"
#include "Config.hpp"

namespace storage
{
    struct FileInfo
    {
        time_t last_access_time_;
        time_t last_change_time_;
        size_t size_of_file_;
        std::string path_;
        std::string url_;

        bool get_file_info(const std::string &path)
        {
            mylog::GetLogger()->Info("get file info start");
            FileUtil f(path);
            if (!f.Exists())
            {
                mylog::GetLogger()->Info("file not exists");
                return false;
            }

            last_access_time_ = f.LastAccessTime();
            last_change_time_ = f.LastChangeTime();
            size_of_file_ = f.FileSize();
            path_ = path;
            url_ = Config::GetInstance()->GetDownloadPrefix() + f.FileName();
        }
    };

    class DataManager
    {
    private:
        DataManager()
        {
            mylog::GetLogger()->Info("DataManager construct start");
            storage_file_ = Config::GetInstance()->GetStorageInfo();
            pthread_rwlock_init(&rwlock_, NULL);
            if (!read_storage_data())
            {
                mylog::GetLogger()->Fatal("get storage data fall");
                perror("get storage data falled!!!: ");
            }
            mylog::GetLogger()->Info("DataManager construct end");
        }

    public:
        ~DataManager()
        {
            pthread_rwlock_destroy(&rwlock_);
        }

        bool read_storage_data()
        {
            mylog::GetLogger()->Info("init datamanager");
            storage::FileUtil f(storage_file_);
            if (!f.Exists())
            {
                mylog::GetLogger()->Info("there is no storage file info need to load");
                return true;
            }

            std::string body;
            if (!f.get_file(&body))
                return false;

            nlohmann::json j = nlohmann::json::parse(body);
            for (auto i = j.begin(); i != j.end(); i++)
            {
                FileInfo temp;
                temp.last_access_time_ = (*i)["last_access_time"];
                temp.last_change_time_ = (*i)["last_change_time"];
                temp.size_of_file_ = (*i)["size_of_file"];
                temp.path_ = (*i)["path"];
                temp.url_ = (*i)["url"];
                map_[temp.url_] = temp;
            }

            return true;
        }

        bool save()
        {
            mylog::GetLogger()->Info("message storage start");
            std::vector<FileInfo> arr;
            if (!get_all_fileinfo(&arr))
            {
                mylog::GetLogger()->Warn("GetAll fail,can't get StorageInfo");
                return false;
            }

            nlohmann::json j;

            for (int i = 0; i < arr.size(); i++)
            {
                nlohmann::json k = nlohmann::json{
                    {"last_access_time", arr[i].last_access_time_},
                    {"last_change_time", arr[i].last_change_time_},
                    {"size_of_file", arr[i].size_of_file_},
                    {"path", arr[i].path_},
                    {"url", arr[i].url_}};
                j.push_back(k);
            }

            std::string body = j.dump(4);
            mylog::GetLogger()->Info("new message for StorageInfo:%s", body.c_str());

            // 写入文件
            FileUtil f(storage_file_);

            if (f.write_file(body.c_str(), body.size()) == false)
                mylog::GetLogger()->Error("SetContent for StorageInfo Error");

            mylog::GetLogger()->Info("message storage end");
            return true;
        }

        bool insert(FileInfo &temp)
        {
            mylog::GetLogger()->Info("data_message Insert start");
            pthread_rwlock_wrlock(&rwlock_); // 加写锁
            map_[temp.url_] = temp;
            pthread_rwlock_unlock(&rwlock_);
            if (save() == false)
            {
                mylog::GetLogger()->Error("data_message Insert:save Error");
                return false;
            }
            mylog::GetLogger()->Info("data_message Insert end");
            return true;
        }

        bool get_all_fileinfo(std::vector<FileInfo> *arry)
        {
            pthread_rwlock_wrlock(&rwlock_);
            for (auto e : map_)
                arry->emplace_back(e.second);
            pthread_rwlock_unlock(&rwlock_);
            return true;
        }

        bool get_fileinfo_url(std::string url, FileInfo *info)
        {
            pthread_rwlock_rdlock(&rwlock_);
            // URL是key，所以直接find()找
            if (map_.find(url) == map_.end())
            {
                pthread_rwlock_unlock(&rwlock_);
                return false;
            }
            *info = map_[url]; // 获取url对应的文件存储信息
            pthread_rwlock_unlock(&rwlock_);
            return true;
        }

    public:
        static DataManager *GetInstance()
        {
            if (instance_ == nullptr)
            {
                mutex_.lock();
                if (instance_ == nullptr)
                {
                    instance_ = new DataManager;
                    return instance_;
                }
                mutex_.unlock();
            }
            return instance_;
        }

    private:
        std::string storage_file_;
        pthread_rwlock_t rwlock_;
        static DataManager *instance_;
        static std::mutex mutex_;
        std::unordered_map<std::string, FileInfo> map_;
    };

}