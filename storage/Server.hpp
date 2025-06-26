#pragma once

#include "DataManager.hpp"
#include "base64.h"

#include <fcntl.h>
#include <regex>
#include <event2/event.h>
#include <event2/http.h>
#include <event2/buffer.h>

namespace storage
{
    class Server
    {
    private:
        int server_port_;
        std::string server_ip_;
        std::string download_prefix_;

    public:
        Server()
        {
            mylog::GetLogger()->Info("Server construct start");
            server_port_ = Config::GetInstance()->GetServerPort();
            server_ip_ = Config::GetInstance()->GetServerIp();
            download_prefix_ = Config::GetInstance()->GetDownloadPrefix();
            mylog::GetLogger()->Info("Server construct finish");
        }

        void service()
        {
            mylog::GetLogger()->Info("service start");
            struct event_base *base = event_base_new();

            if (base == NULL)
            {
                mylog::GetLogger()->Fatal("event_base_new err!");
                return;
            }

            sockaddr_in sin;
            memset(&sin, 0, sizeof(sin));
            sin.sin_family = AF_INET;
            sin.sin_port = htons(server_port_);

            evhttp *httpd = evhttp_new(base);

            if (evhttp_bind_socket(httpd, "0.0.0.0", server_port_) != 0)
            {
                mylog::GetLogger()->Fatal("bind http socket err!!");
                return;
            }

            evhttp_set_gencb(httpd, callback, NULL);

            if (base && -1 == event_base_dispatch(base))
            {
                mylog::GetLogger()->Debug("event_base_dispatch err");
            }
            if (base)
                event_base_free(base);
            if (httpd)
                evhttp_free(httpd);
            return;
        }

    private:
        static void callback(struct evhttp_request *req, void *args)
        {
            std::string path = evhttp_uri_get_path(evhttp_request_get_evhttp_uri(req));
            mylog::GetLogger()->Info("get req, uri: %s", path.c_str());

            if (path.find("/download/") != std::string::npos)
            {
                Download(req, args);
            }
            else if (path == "/upload")
            {
                Upload(req, args);
            }
            else if (path == "/")
            {
                ListShow(req, args);
            }
            else
            {
                evhttp_send_reply(req, HTTP_NOTFOUND, "Not Found", NULL);
            }
        }

        static std::string TimetoStr(time_t t)
        {
            std::string tmp = std::ctime(&t);
            return tmp;
        }

        // 文件大小格式化函数
        static std::string formatSize(uint64_t bytes)
        {
            const char *units[] = {"B", "KB", "MB", "GB"};
            int unit_index = 0;
            double size = bytes;

            while (size >= 1024 && unit_index < 3)
            {
                size /= 1024;
                unit_index++;
            }

            std::stringstream ss;
            ss << std::fixed << std::setprecision(2) << size << " " << units[unit_index];
            return ss.str();
        }

        // 前端代码处理函数在渲染函数中直接处理StorageInfo
        static std::string generateModernFileList(const std::vector<FileInfo> &files)
        {
            std::stringstream ss;
            ss << "<div class='file-list'><h3>已上传文件</h3>";

            for (const auto &file : files)
            {
                std::string filename = FileUtil(file.path_).FileName();

                // 从路径中解析存储类型（示例逻辑，需根据实际路径规则调整）
                std::string storage_type = "low";
                if (file.path_.find("deep") != std::string::npos)
                {
                    storage_type = "deep";
                }

                ss << "<div class='file-item'>"
                   << "<div class='file-info'>"
                   << "<span>📄" << filename << "</span>"
                   << "<span class='file-type'>"
                   << (storage_type == "deep" ? "深度存储" : "普通存储")
                   << "</span>"
                   << "<span>" << formatSize(file.size_of_file_) << "</span>"
                   << "<span>" << TimetoStr(file.last_access_time_) << "</span>"
                   << "</div>"
                   << "<button onclick=\"window.location='" << file.url_ << "'\">⬇️ 下载</button>"
                   << "</div>";
            }

            ss << "</div>";
            return ss.str();
        }

        // 为客户显示界面，发送html文件内容
        static void ListShow(struct evhttp_request *req, void *args)
        {
            mylog::GetLogger()->Info("ListShow()");

            // 获取所有已存放的文件
            std::vector<FileInfo> arr;
            DataManager::GetInstance()->get_all_fileinfo(&arr);

            // 读取html模板
            std::ifstream template_html("template.html");
            std::string html_str((std::istreambuf_iterator<char>(template_html)),
                                 std::istreambuf_iterator<char>());

            // 替换文件列表
            html_str = std::regex_replace(html_str,
                                          std::regex("\\{\\{FILE_LIST\\}\\}"),
                                          generateModernFileList(arr));

            // 替换服务器地址
            html_str = std::regex_replace(html_str,
                                          std::regex("\\{\\{BACKEND_URL\\}\\}"),
                                          "http://" + storage::Config::GetInstance()->GetServerIp() + ":" + std::to_string(storage::Config::GetInstance()->GetServerPort()));

            struct evbuffer *buf = evhttp_request_get_output_buffer(req);
            evbuffer_add(buf, (const void *)html_str.c_str(), html_str.size());
            evhttp_add_header(evhttp_request_get_output_headers(req), "Content_Type", "text/html;charset=utf-8");
            evhttp_send_reply(req, HTTP_OK, NULL, NULL);
            mylog::GetLogger()->Info("ListShow() finish");
        }

        static std::string GetETag(FileInfo info)
        {
            FileUtil fu(info.path_);
            std::string etag = fu.FileName();
            etag += "-";
            etag += std::to_string(info.size_of_file_);
            etag += "-";
            etag += std::to_string(info.last_change_time_);
            return etag;
        }

        // 下载文件
        static void Download(struct evhttp_request *req, void *agrs)
        {
            mylog::GetLogger()->Info("Download Start");
            // 先获取用户下载文件路径
            FileInfo finfo;
            std::string source_path = evhttp_uri_get_path(evhttp_request_get_evhttp_uri(req));
            source_path = UrlDecode(source_path);
            DataManager::GetInstance()->get_fileinfo_url(source_path, &finfo);
            mylog::GetLogger()->Info("request resource_path:%s", source_path.c_str());

            std::string storage_path = finfo.path_;
            FileUtil download_file(storage_path);

            // 判断这个文件是否被压缩，如果被压缩，解压到新的文件
            if (storage_path.find(Config::GetInstance()->GetDeepStorageDir()) != std::string::npos)
            {
                mylog::GetLogger()->Info("uncompressing: %s", storage_path.c_str());
                // 创建一个uncompress文件夹存放解压后的文件
                storage_path = "uncompress" + std::string(finfo.path_.begin() + finfo.path_.find_last_of('/') + 1, finfo.path_.end());
                FileUtil dir_create("uncompress");
                dir_create.creat_dir();
                download_file.uncompress(storage_path);
                download_file = FileUtil(storage_path);
            }

            if (finfo.path_.find(Config::GetInstance()->GetDeepStorageDir()) != std::string::npos && !download_file.Exists())
            {
                // 如果解压后文件不存在，是服务端解压出错
                mylog::GetLogger()->Info("evhttp_send_reply: 500 - UnCompress failed");
                evhttp_send_reply(req, HTTP_INTERNAL, NULL, NULL);
            }
            else if (finfo.path_.find(Config::GetInstance()->GetLowStorageDir()) != std::string::npos && !download_file.Exists())
            {
                // 如果是普通文件，且不存在，是客户端出错
                mylog::GetLogger()->Info("evhttp_send_reply: 400 - bad request,file not exists");
                evhttp_send_reply(req, HTTP_BADREQUEST, "file not exists", NULL);
            }

            // 确认是否需要断点续传
            bool retrans = false;
            std::string old_etag;
            auto if_range = evhttp_find_header(evhttp_request_get_input_headers(req), "If-Range");
            if (NULL != if_range)
            {
                old_etag = if_range;
                // 有If-Range字段且，这个字段的值与请求文件的最新etag一致则符合断点续传
                if (old_etag == GetETag(finfo))
                {
                    retrans = true;
                    mylog::GetLogger()->Info("%s need breakpoint continuous transmission", storage_path.c_str());
                }
            }

            // 4. 读取文件数据，放入rsp.body中
            if (download_file.Exists() == false)
            {
                mylog::GetLogger()->Info("%s not exists", storage_path.c_str());
                storage_path += "not exists";
                evhttp_send_reply(req, 404, storage_path.c_str(), NULL);
                return;
            }
            evbuffer *outbuf = evhttp_request_get_output_buffer(req);
            int fd = open(storage_path.c_str(), O_RDONLY);
            if (fd == -1)
            {
                mylog::GetLogger()->Error("open file error: %s -- %s", storage_path.c_str(), strerror(errno));
                evhttp_send_reply(req, HTTP_INTERNAL, strerror(errno), NULL);
                return;
            }
            // 和前面用的evbuffer_add类似，但是效率更高，具体原因可以看函数声明
            if (-1 == evbuffer_add_file(outbuf, fd, 0, download_file.FileSize()))
            {
                mylog::GetLogger()->Error("evbuffer_add_file: %d -- %s -- %s", fd, storage_path.c_str(), strerror(errno));
            }
            // 5. 设置响应头部字段： ETag， Accept-Ranges: bytes
            evhttp_add_header(evhttp_request_get_output_headers(req), "Accept-Ranges", "bytes");
            evhttp_add_header(evhttp_request_get_output_headers(req), "ETag", GetETag(finfo).c_str());
            evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/octet-stream");
            if (retrans == false)
            {
                evhttp_send_reply(req, HTTP_OK, "Success", NULL);
                mylog::GetLogger()->Info("evhttp_send_reply: HTTP_OK");
            }
            else
            {
                evhttp_send_reply(req, 206, "breakpoint continuous transmission", NULL); // 区间请求响应的是206
                mylog::GetLogger()->Info("evhttp_send_reply: 206");
            }
            if (storage_path != finfo.path_)
            {
                remove(storage_path.c_str()); // 删除文件
            }
        }

        static void Upload(struct evhttp_request *req, void *args)
        {
            mylog::GetLogger()->Info("Upload Start");

            // 获取inputbuffer
            struct evbuffer *input_buffer = evhttp_request_get_input_buffer(req);
            if (input_buffer == nullptr)
            {
                mylog::GetLogger()->Info("input_buffer is empty");
                return;
            }

            // 获取请求体长度
            size_t len = evbuffer_get_length(input_buffer);
            if (len == 0)
            {
                mylog::GetLogger()->Info("buffer is empty");
                evhttp_send_reply(req, HTTP_BADREQUEST, "empty file", NULL);
                return;
            }

            // 获取文件
            std::string content(len, 0);
            if (-1 == evbuffer_copyout(input_buffer, (void *)content.c_str(), len))
            {
                mylog::GetLogger()->Error("evbuffer_copyout error");
                evhttp_send_reply(req, HTTP_INTERNAL, NULL, NULL);
                return;
            }

            // 获取存储类型
            std::string storage_type = evhttp_find_header(evhttp_request_get_input_headers(req), "StorageType");

            // 获取文件名
            std::string file_name = evhttp_find_header(evhttp_request_get_input_headers(req), "FileName");
            file_name = base64_decode(file_name);

            // 存储路径
            std::string storage_path;
            if (storage_path == "low")
            {
                storage_path = Config::GetInstance()->GetLowStorageDir();
                mylog::GetLogger()->Info("%s--LowStorage", file_name);
            }
            else if (storage_path == "deep")
            {
                storage_path = Config::GetInstance()->GetDeepStorageDir();
                mylog::GetLogger()->Info("%s--DeepStorage", file_name);
            }
            else
            {
                mylog::GetLogger()->Error("storage type illegal");
                evhttp_send_reply(req, HTTP_BADREQUEST, "storage type illegal", NULL);
                return;
            }

            // 如果不存在就创建low或deep目录
            FileUtil dirCreate(storage_path);
            dirCreate.creat_dir();

            storage_path += file_name;

            FileUtil fu(storage_path);

            // 如果是Deep，需要压缩
            if (storage_type == "deep" && !fu.compress(content, Config::GetInstance()->GetBundleFormat()))
            {
                evhttp_send_reply(req, HTTP_INTERNAL, "conpress error", NULL);
                return;
            }

            //写入文件
            if(!fu.write_file(content.c_str(), content.size()))
            {
                mylog::GetLogger()->Error("write file error");
                evhttp_send_reply(req, HTTP_INTERNAL, "write file error", NULL);
                return;
            }

            //文件写入完毕
            mylog::GetLogger()->Info("%sstorage success", storage_type);

            //创建文件info
            FileInfo info;
            info.get_file_info(storage_path);
            DataManager::GetInstance()->insert(info);
            evhttp_send_reply(req, HTTP_OK, "success", NULL);
            mylog::GetLogger()->Info("Upload success");

        }
    };
}