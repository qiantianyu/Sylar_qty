#include "log.h"
#include<iostream>
#include<map>


namespace sylar{

    const char* LogLevel::ToString(LogLevel::Level level) {
        //https://blog.csdn.net/qq_41673920/article/details/115473160
        switch(level) {
        #define XX(name)         \
            case LogLevel::name: \
                return #name;    \
                break;
                
            XX(DEBUG);
            XX(INFO);
            XX(WARN);
            XX(ERROR);
            XX(FATAL);

        #undef XX
            default:
            return "UNKNOW";
        }
        return "UNKNOW";
    }

    class MessageFormatItem : public LogFormatter::FormatItem {
    public:
        void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
            os << event->getContent();
        }

    };

    class LevelFormatItem : public LogFormatter::FormatItem {
    public:
        void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
            os << LogLevel::ToString(level);
        }

    };

    class ElapseFormatItem : public LogFormatter::FormatItem {
    public:
        void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
            os << event->getElapse();
        }

    };

    class NameFormatItem : public LogFormatter::FormatItem {
    public:
        void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
            os << logger->getName();
        }

    };

    class ThreadIdFormatItem : public LogFormatter::FormatItem {
    public:
        void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
            os << event->getThreadId();
        }

    };

    class FiberIdFormatItem : public LogFormatter::FormatItem {
    public:
        void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
            os << event->getFiberId();
        }

    };

    class DataTimeFormatItem : public LogFormatter::FormatItem {
    public:
        DataTimeFormatItem(const std::string& format = "%Y:%m:%d %H:%M:%s") :m_format(format) {

        }
        void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
            os << event->getTime();
        }

    private:
        std::string m_format;
    };

    class FilenameFormatItem : public LogFormatter::FormatItem {
    public:
        void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
            os << event->getFile();
        }

    };

    class LineFormatItem : public LogFormatter::FormatItem {
    public:
        void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
            os << event->getLine();
        }

    };

    Logger::Logger(const std::string& name):m_name(name) {
        
    }

    void Logger::addAppender(LogAppender::ptr appender) {
        m_appenders.push_back(appender);
    }

    void Logger::delAppender(LogAppender::ptr appender) {
        for(auto it = m_appenders.begin();it!=m_appenders.end();++it) {
            if(*it==appender){
                m_appenders.erase(it);
                break;
            }
        }
    }

    void Logger::log(LogLevel::Level level,LogEvent::ptr event) {
        if(level>=m_level){
            for(auto& i:m_appenders){
                i->log(level, event);
            }
        }
    }
    
    void Logger::debug(LogEvent::ptr event) {
        log(LogLevel::DEBUG, event);
    }
    
    void Logger::info(LogEvent::ptr event) {
        log(LogLevel::INFO, event);
    }
    
    void Logger::warn(LogEvent::ptr event) {
        log(LogLevel::WARN, event);
    }
    
    void Logger::error(LogEvent::ptr event) {
        log(LogLevel::ERROR, event);
    }
        
    void Logger::fatal(LogEvent::ptr event) {
        log(LogLevel::FATAL, event);
    }


//LogAppender类的两个子类的成员函数
    FileLogAppender::FileLogAppender(const std::string& filename) :m_filename(filename) {

    }

    void FileLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level,LogEvent::ptr event) {
        if(level>=m_level) {
            m_filestream << m_formatter->format(logger,level,event); // 注意这里m_formatter是智能指针，要用->调用成员函数
        }
    }

    bool FileLogAppender::reopen() {
        if(m_filestream) {
            m_filestream.close();
        }
        m_filestream.open(m_filename);
        return !!m_filestream; // 这里！！相当于转换数据类型，使得非0值转换为1，0还是0，不然返回值不是bool类型
    }


    void StdoutLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level,LogEvent::ptr event) {
        if(level>=m_level){
            std::cout << m_formatter->format(logger, level, event);
        }
    }

    LogFormatter::LogFormatter(const std::string& pattern):m_pattern(pattern) {

    }
    
    std::string LogFormatter::format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) {
        std::stringstream ss;
        for(auto& i:m_items){
            i->format(ss,logger, level, event);
        }
        return ss.str();
    }
 //%xxx %xxx(xxx) %%
    void LogFormatter::init() {
        std::vector<std::tuple<std::string, std::string, int>> vec;
        std::string nstr;
        for(size_t i = 0;i<m_pattern.size();++i){
            if(m_pattern[i] != '%') {
                nstr.append(1, m_pattern[i]);
                continue;
            }
            
            if((i+1)<m_pattern.size()){
                if(m_pattern[i+1] == '%') {
                    nstr.append(1, '%');
                    continue;
                }
            }
            
            size_t n = i + 1;
            int fmt_status = 0;
            size_t fmt_begin = 0;

            std::string str;
            std::string fmt; 
    
            while(n < m_pattern.size()) {
                if(isspace(m_pattern[n])) {
                    break;
                }
                if(fmt_status == 0) {
                    if(m_pattern[n] == '{') {
                        str = m_pattern.substr(i + 1, n - i);
                        fmt_status = 1; // 解析格式
                        fmt_begin = n;
                        ++n;
                        continue;
                    }
                }
                if(fmt_status == 1) {
                    if(m_pattern[n] == '}') {
                        fmt = m_pattern.substr(fmt_begin + 1, n - fmt_begin - 1);
                        fmt_status = 2;
                        break;
                    }
                }
            }
            if(fmt_status == 0) {
                if(!nstr.empty()) {
                    vec.push_back(std::make_tuple(nstr, std::string(), 0));
                }
                str = m_pattern.substr(i + 1, n - i);
                vec.push_back(std::make_tuple(str, fmt, 1));
                i = n;
            }
            else if(fmt_status == 1) {
                std::cout << "pattern parse error" << m_pattern << "-" << m_pattern.substr(i) << std::endl;
                vec.push_back(std::make_tuple("<<pattern_error", fmt, 0));
            }
            else if (fmt_status == 2) {
                if(!nstr.empty()) {
                    vec.push_back(std::make_tuple(nstr, "", 0));
                }
                vec.push_back(std::make_tuple(str, fmt, 1));
                i = n;
            }
        }

        if(!nstr.empty()) {
            vec.push_back(std::make_tuple(nstr, "", 0));
        }

        static std::map<std::string, std::function<FormatItem::ptr(const std::string &str)>> s_format_items = {

        };
    }

}