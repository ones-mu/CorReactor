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
        // virtual std::string getTypeName() const = 0;

    protected:
        /// 配置参数的名称
        std::string m_name;
        /// 配置参数的描述
        std::string m_description;
    };
    /**
     * @brief 配置参数模板子类,保存对应类型的参数值
     * @details T 参数的具体类型
     *          FromStr 从std::string转换成T类型的仿函数
     *          ToStr 从T转换成std::string的仿函数
     *          std::string 为YAML格式的字符串
     */
    template <class T>
    class ConfigVar : public ConfigVarBase
    {
    public:
        using RWMutexType = version04::RWMutex;
        using ptr = std::shared_ptr<ConfigVar<T>>;
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
                return boost::lexical_cast<std::string>(m_val);
            }
            catch (const std::exception &e)
            {
                ULOG_INFO_SRC("system", "ConfigVar<T> toStirng() exception:e.what()={} convert={} to string;", e.what(), typeid(m_val).name());
            }
        }
        bool fromString(const std::string &val) override
        {
            try
            {
                RWMutexType::WriteLock lock(m_mutex);
                m_val = boost::lexical_cast<T>(val);
                return true;
            }
            catch (const std::exception &e)
            {
                ULOG_INFO_SRC("system", "ConfigVar<T> toStirng() exception:e.what()={} convert string to{}-{};", e.what(), typeid(m_val).name(), val);
            }
        }
        const T getValue() const { return m_val; }
        void setValue(const T& v){m_val = v;}

    private:
        RWMutexType m_mutex;
        T m_val;
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
        static typename ConfigVar<T>::ptr Lookup(const std::string &name, const T &default_value, const std::string &description = "")
        {
            auto tmp = Config::Lookup<T>(name);
            if (tmp)
            {
                ULOG_INFO_SRC("main", "Look up name={} exists", name);
                return tmp;
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
        inline static ConfigVarMap s_datas;//c++17之后可以使用inline，就不用在类外初始化了
        inline static RWMutexType s_mutex;
    };
}