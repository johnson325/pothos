// Copyright (c) 2013-2016 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Object/Object.hpp>
#include <Pothos/Object/Exception.hpp>
#include <Pothos/Callable.hpp>
#include <Pothos/Plugin.hpp>
#include <Pothos/Util/CompareTo.hpp>
#include <Poco/SingletonHolder.h>
#include <Poco/RWLock.h>
#include <Poco/Logger.h>
#include <Poco/Format.h>
#include <Poco/Hash.h>
#include <map>

/***********************************************************************
 * Global map structure for comparisons
 **********************************************************************/
static Poco::RWLock &getMapMutex(void)
{
    static Poco::SingletonHolder<Poco::RWLock> sh;
    return *sh.get();
}

//singleton global map for all supported comparisons
typedef std::map<size_t, Pothos::Plugin> CompareMapType;
static CompareMapType &getCompareMap(void)
{
    static Poco::SingletonHolder<CompareMapType> sh;
    return *sh.get();
}

//! combine two type hashes to form a unique hash such that hash(a, b) != hash(b, a)
static inline size_t typesHashCombine(const std::type_info &t0, const std::type_info &t1)
{
    return t0.hash_code() ^ Poco::hash(t1.hash_code());
}

/***********************************************************************
 * Comparison registration handling
 **********************************************************************/
static void handleComparePluginEvent(const Pothos::Plugin &plugin, const std::string &event)
{
    poco_debug_f2(Poco::Logger::get("Pothos.Object.handleComparePluginEvent"), "plugin %s, event %s", plugin.toString(), event);
    POTHOS_EXCEPTION_TRY
    {
        //validate the plugin -- if we want to handle it -- check the signature:
        if (plugin.getObject().type() != typeid(Pothos::Callable)) return;
        const auto &call = plugin.getObject().extract<Pothos::Callable>();
        if (call.type(-1) != typeid(int)) return;
        if (call.getNumArgs() != 2) return;

        //extract type info used for map lookup
        const std::type_info &t0 = call.type(0);
        const std::type_info &t1 = call.type(1);

        Poco::RWLock::ScopedWriteLock lock(getMapMutex());
        if (event == "add")
        {
            getCompareMap()[typesHashCombine(t0, t1)] = plugin;
        }
        if (event == "remove")
        {
            getCompareMap()[typesHashCombine(t0, t1)] = Pothos::Plugin();
        }
    }
    POTHOS_EXCEPTION_CATCH(const Pothos::Exception &ex)
    {
        poco_error_f3(Poco::Logger::get("Pothos.Object.handleComparePluginEvent"),
            "exception %s, plugin %s, event %s", ex.displayText(), plugin.toString(), event);
    }
}

/***********************************************************************
 * Register event handler
 **********************************************************************/
pothos_static_block(pothosObjectCompareRegister)
{
    Pothos::PluginRegistry::addCall("/object/compare", &handleComparePluginEvent);
}

/***********************************************************************
 * The compare implementation
 **********************************************************************/
int Pothos::Object::compareTo(const Pothos::Object &other) const
{
    //find the plugin in the map, it will be null if not found
    Poco::RWLock::ScopedReadLock lock(getMapMutex());
    auto it = getCompareMap().find(typesHashCombine(this->type(), other.type()));

    //try a number type just in the case that this is possible
    if (it == getCompareMap().end()) try
    {
        return Pothos::Util::compareTo(this->convert<double>(), other.convert<double>());
    }
    catch(const Pothos::ObjectConvertError &){}

    //thow an error when the compare is not supported
    if (it == getCompareMap().end()) throw Pothos::ObjectCompareError(
        "Pothos::Object::compareTo()",
        Poco::format("not supported for %s, %s",
        this->toString(), other.toString()));

    Object args[2];
    args[0] = *this;
    args[1] = other;
    auto call = it->second.getObject().extract<Pothos::Callable>();
    return call.opaqueCall(args, 2).extract<int>();
}
