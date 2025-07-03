/**
 * @file config.h
 * @brief 配置模块
配置模块采用约定优于配置配置模块采用约定优于配置的设计思想，
让程序所依赖的配置项都有一个默认值，就不需要每次都指定了，这样既简单又灵活。
当文件配置项做出改变时，也会改变相应的配置参数
 */
#pragma once

#include "controllogger.h"
#include "thread.h"
#include "macro.h"

#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include <list>
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>
#include <fstream>
#include <iostream>
#include <functional>

#include <boost/lexical_cast.hpp> //类型转换
#include <yaml-cpp/yaml.h>

namespace version04
{
    /**
     * @brief 配置变量的基类
     * 定义一个基类，用于放公用的属性
     */
    class ConfigVarBase
    {
    public:
        typedef std::shared_ptr<ConfigVarBase> ptr;
        /**
         * @brief 构造函数
         * @param[in] name 配置参数名称[0-9a-z_.]
         * @param[in] description 配置参数描述
         */
        ConfigVarBase(const std::string &name, const std::string &description = "")
            : m_name(name), m_description(description)
        {
            std::transform(m_name.begin(), m_name.end(), m_name.begin(), ::tolower);
        }

        /**
         * @brief 析构函数
         */
        virtual ~ConfigVarBase() {}

        /**
         * @brief 返回配置参数名称
         */
        const std::string &getName() const { return m_name; }

        /**
         * @brief 返回配置参数的描述
         */
        const std::string &getDescription() const { return m_description; }

        /**
         * @brief 转成字符串
         */
        virtual std::string toString() = 0;

        /**
         * @brief 从字符串初始化值
         */
        virtual bool fromString(const std::string &val) = 0;

        /**
         * @brief 返回配置参数值的类型名称
         */
        virtual std::string getTypeName() const = 0;

    protected:
        /// 配置参数的名称
        std::string m_name;
        /// 配置参数的描述
        std::string m_description;
    };

    /**
     * @brief 类型转换模板类(F 源类型, T 目标类型)
     */
    template <typename F, typename T>
    class LexicalCast
    {
    public:
        /**
         * @brief 类型转换
         * @param[in] v 源类型值
         * @return 返回v转换后的目标类型
         * @exception 当类型不可转换时抛出异常
         */
        T operator()(const F &v)
        {
            return boost::lexical_cast<T>(v);
        }
    };

    /**
     * @brief 类型转换模板类片特化(YAML String 转换成 std::vector<T>)
     */
    template <class T>
    class LexicalCast<std::string, std::vector<T>>
    {
    public:
        std::vector<T> operator()(const std::string &v)
        {
            YAML::Node node = YAML::Load(v);
            typename std::vector<T> vec;
            std::stringstream ss;
            for (size_t i = 0; i < node.size(); ++i)
            {
                ss.str("");
                ss << node[i];
                vec.push_back(LexicalCast<std::string, T>()(ss.str()));
            }
            return vec;
        }
    };

    /**
     * @brief 类型转换模板类片特化(std::vector<T> 转换成 YAML String)
     */
    template <class T>
    class LexicalCast<std::vector<T>, std::string>
    {
    public:
        std::string operator()(const std::vector<T> &v)
        {
            YAML::Node node;
            for (auto &i : v)
            {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    /**
     * @brief 类型转换模板类片特化(YAML String 转换成 std::list<T>)
     */
    template <class T>
    class LexicalCast<std::string, std::list<T>>
    {
    public:
        std::list<T> operator()(const std::string &v)
        {
            YAML::Node node = YAML::Load(v);
            typename std::list<T> vec;
            std::stringstream ss;
            for (size_t i = 0; i < node.size(); ++i)
            {
                ss.str("");
                ss << node[i];
                vec.push_back(LexicalCast<std::string, T>()(ss.str()));
            }
            return vec;
        }
    };

    /**
     * @brief 类型转换模板类片特化(std::list<T> 转换成 YAML String)
     */
    template <class T>
    class LexicalCast<std::list<T>, std::string>
    {
    public:
        std::string operator()(const std::list<T> &v)
        {
            YAML::Node node;
            for (auto &i : v)
            {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    /**
     * @brief 类型转换模板类片特化(YAML String 转换成 std::set<T>)
     */
    template <class T>
    class LexicalCast<std::string, std::set<T>>
    {
    public:
        std::set<T> operator()(const std::string &v)
        {
            YAML::Node node = YAML::Load(v);
            typename std::set<T> vec;
            std::stringstream ss;
            for (size_t i = 0; i < node.size(); ++i)
            {
                ss.str("");
                ss << node[i];
                vec.insert(LexicalCast<std::string, T>()(ss.str()));
            }
            return vec;
        }
    };

    /**
     * @brief 类型转换模板类片特化(std::set<T> 转换成 YAML String)
     */
    template <class T>
    class LexicalCast<std::set<T>, std::string>
    {
    public:
        std::string operator()(const std::set<T> &v)
        {
            YAML::Node node;
            for (auto &i : v)
            {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    /**
     * @brief 类型转换模板类片特化(YAML String 转换成 std::unordered_set<T>)
     */
    template <class T>
    class LexicalCast<std::string, std::unordered_set<T>>
    {
    public:
        std::unordered_set<T> operator()(const std::string &v)
        {
            YAML::Node node = YAML::Load(v);
            typename std::unordered_set<T> vec;
            std::stringstream ss;
            for (size_t i = 0; i < node.size(); ++i)
            {
                ss.str("");
                ss << node[i];
                vec.insert(LexicalCast<std::string, T>()(ss.str()));
            }
            return vec;
        }
    };

    /**
     * @brief 类型转换模板类片特化(std::unordered_set<T> 转换成 YAML String)
     */
    template <class T>
    class LexicalCast<std::unordered_set<T>, std::string>
    {
    public:
        std::string operator()(const std::unordered_set<T> &v)
        {
            YAML::Node node;
            for (auto &i : v)
            {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    /**
     * @brief 类型转换模板类片特化(YAML String 转换成 std::map<std::string, T>)
     */
    template <class T>
    class LexicalCast<std::string, std::map<std::string, T>>
    {
    public:
        std::map<std::string, T> operator()(const std::string &v)
        {
            YAML::Node node = YAML::Load(v);
            typename std::map<std::string, T> vec;
            std::stringstream ss;
            for (auto it = node.begin();
                 it != node.end(); ++it)
            {
                ss.str("");
                ss << it->second;
                vec.insert(std::make_pair(it->first.Scalar(),
                                          LexicalCast<std::string, T>()(ss.str())));
            }
            return vec;
        }
    };

    /**
     * @brief 类型转换模板类片特化(std::map<std::string, T> 转换成 YAML String)
     */
    template <class T>
    class LexicalCast<std::map<std::string, T>, std::string>
    {
    public:
        std::string operator()(const std::map<std::string, T> &v)
        {
            YAML::Node node;
            for (auto &i : v)
            {
                node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    /**
     * @brief 类型转换模板类片特化(YAML String 转换成 std::unordered_map<std::string, T>)
     */
    template <class T>
    class LexicalCast<std::string, std::unordered_map<std::string, T>>
    {
    public:
        std::unordered_map<std::string, T> operator()(const std::string &v)
        {
            YAML::Node node = YAML::Load(v);
            typename std::unordered_map<std::string, T> vec;
            std::stringstream ss;
            for (auto it = node.begin();
                 it != node.end(); ++it)
            {
                ss.str("");
                ss << it->second;
                vec.insert(std::make_pair(it->first.Scalar(),
                                          LexicalCast<std::string, T>()(ss.str())));
            }
            return vec;
        }
    };

    /**
     * @brief 类型转换模板类片特化(std::unordered_map<std::string, T> 转换成 YAML String)
     */
    template <class T>
    class LexicalCast<std::unordered_map<std::string, T>, std::string>
    {
    public:
        std::string operator()(const std::unordered_map<std::string, T> &v)
        {
            YAML::Node node;
            for (auto &i : v)
            {
                node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    /**
     * @brief 配置参数模板子类,保存对应类型的参数值
     * @details T 参数的具体类型
     *          FromStr 从std::string转换成T类型的仿函数
     *          ToStr 从T转换成std::string的仿函数
     *          std::string 为YAML格式的字符串
     * 仿函数 运算符重载
     * FromStr T operator()(const std::string& v) const;
     * ToStr std::string operator()(const T& v) const;
     */
    template <class T, class FromStr = LexicalCast<std::string, T>, class ToStr = LexicalCast<T, std::string>>
    class ConfigVar : public ConfigVarBase
    {
    public:
        using RWMutexType = version04::RWMutex;
        using ptr = std::shared_ptr<ConfigVar<T>>;
        using on_change_cb = std::function<void(const T &old_val, const T &new_val)>;
        /**
         * @brief 通过参数名,参数值,描述构造ConfigVar
         * @param[in] name 参数名称有效字符为[0-9a-z_.]
         * @param[in] default_value 参数的默认值
         * @param[in] description 参数的描述
         */
        ConfigVar(const std::string &name, const T &default_value, const std::string &description = "")
            : ConfigVarBase(name, description), m_val(default_value) {}

        std::string toString() override
        {
            try
            {
                RWMutexType::ReadLock lock(m_mutex);
                // return boost::lexical_cast<std::string>(m_val);
                return ToStr()(m_val);
            }
            catch (const std::exception &e)
            {
                ULOG_INFO_SRC("system", "ConfigVar<T> toStirng() exception:e.what()={} convert={} to string;", e.what(), typeid(m_val).name());
            }
            return "";
        }
        bool fromString(const std::string &val) override
        {
            try
            {
                setValue(FromStr()(val));
                // m_val = boost::lexical_cast<T>(val);
                return true;
            }
            catch (const std::exception &e)
            {
                ULOG_INFO_SRC("system", "ConfigVar<T> toStirng() exception:e.what()={} convert string to{}-{};", e.what(), typeid(m_val).name(), val);
            }
        }
        const T getValue()
        {
            RWMutexType::ReadLock lock(m_mutex);
            return m_val;
        }
        void setValue(const T &v)
        {
            {
                RWMutexType::ReadLock lock(m_mutex);
                if (v == m_val)
                {
                    return;
                }
                for (auto &i : m_cbs)
                {
                    //对每个回调函数，传入当前的键值
                    i.second(m_val, v);
                }
            }
            RWMutexType::WriteLock lock(m_mutex);
            m_val = v;
        }
        std::string getTypeName() const override { return typeid(T).name(); }
        uint64_t addListener(on_change_cb cb)
        {
            static uint64_t s_fun_id = 0;
            RWMutexType::WriteLock lock(m_mutex);
            ++s_fun_id;
            m_cbs[s_fun_id] = cb;
            return s_fun_id;
        }
        void delListener(uint64_t key)
        {
            RWMutexType::WriteLock lock(m_mutex);
            m_cbs.erase(key);
        }
        on_change_cb getListener(uint64_t key)
        {
            RWMutexType::ReadLock lock(m_mutex);    
            auto it = m_cbs.find(key);
            return it == m_cbs.end() ? nullptr : it->second;
        }
        void clearListener()
        {
            RWMutexType::WriteLock lock(m_mutex);
            m_cbs.clear();
        }

    private:
        RWMutexType m_mutex;
        T m_val;
        // 变更回调函数组, uint64_t key要求唯一，一般可以用hash
        std::map<uint64_t, on_change_cb> m_cbs;
    };
    /**
     * @brief ConfigVar的管理类
     * @details 提供便捷的方法创建/访问ConfigVar
     */
    class Config
    {
    public:
        using ConfigVarMap = std::unordered_map<std::string, ConfigVarBase::ptr>;
        using RWMutexType = version04::RWMutex;

        /**
         * @brief 获取/创建对应参数名的配置参数
         * @param[in] name 配置参数名称
         * @param[in] default_value 参数默认值
         * @param[in] description 参数描述
         * @details 获取参数名为name的配置参数,如果存在直接返回
         *          如果不存在,创建参数配置并用default_value赋值
         * @return 返回对应的配置参数,如果参数名存在但是类型不匹配则返回nullptr
         * @exception 如果参数名包含非法字符[^0-9a-z_.] 抛出异常 std::invalid_argument
         */
        template <typename T>
        static typename ConfigVar<T>::ptr Create(const std::string &name, const T &default_value, const std::string &description = "")
        {
            // auto tmp = Config::Lookup<T>(name);
            // if (tmp)
            // {
            //     ULOG_INFO_SRC("main", "Look up name={} exists", name);
            //     return tmp;
            // }
            {
                RWMutexType::ReadLock lock(s_mutex);
                auto it = s_datas.find(name);
                if (it != s_datas.end())
                {
                    auto tmp = std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
                    if (tmp)
                    {
                        ULOG_INFO_SRC("main", "Look up name={} exists", name);
                        return tmp;
                    }
                    else
                    {
                        ULOG_ERROR_SRC("main", "Look up name={} exists but type not match , typeid(T).name()={},real_type={} {}", name, typeid(T).name(), it->second->getTypeName(), it->second->toString());
                        return nullptr;
                    }
                }
            }
            if (name.find_first_not_of("0123456789abcdefghijklmnopqrstuvwxyz_.") != std::string::npos)
            {
                ULOG_ERROR_SRC("system", "Look up name invalid name={}", name);
                VERSION04_ASSERT2(false, "name invalid");
            }
            typename ConfigVar<T>::ptr v(new ConfigVar<T>(name, default_value, description));
            RWMutexType::WriteLock lock(s_mutex);
            s_datas[name] = v;
            return v;
        }
        /**
         * @brief 查找配置参数
         * @param[in] name 配置参数名称
         * @return 返回配置参数名为name的配置参数
         */
        template <typename T>
        static typename ConfigVar<T>::ptr Lookup(const std::string &name)
        {
            RWMutexType::ReadLock lock(s_mutex);
            auto it = s_datas.find(name);
            if (it == s_datas.end())
            {
                return nullptr;
            }
            return std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
        }

        static void LoadFromYaml(const YAML::Node &root);
        static ConfigVarBase::ptr LookupBase(const std::string &name);

    public:
        inline static ConfigVarMap s_datas; // c++17之后可以使用inline，就不用在类外初始化了
        inline static RWMutexType s_mutex;
    };
}