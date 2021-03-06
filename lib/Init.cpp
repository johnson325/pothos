// Copyright (c) 2013-2016 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Init.hpp>
#include <Pothos/Exception.hpp>
#include <Pothos/System/Paths.hpp>
#include <Pothos/Plugin/Loader.hpp>
#include <Poco/Path.h>
#include <Poco/File.h>
#include <Poco/Format.h>
#include <Poco/SingletonHolder.h>

/***********************************************************************
 * Singleton for initialization once per process
 **********************************************************************/
namespace Pothos {
struct InitSingleton
{
    InitSingleton(void);
};
} //namespace Pothos

static Pothos::InitSingleton &getInitSingleton(void)
{
    static Poco::SingletonHolder<Pothos::InitSingleton> sh;
    return *sh.get();
}

static std::vector<Pothos::PluginModule> &getLoadedModules(void)
{
    static Poco::SingletonHolder<std::vector<Pothos::PluginModule>> sh;
    return *sh.get();
}

void Pothos::init(void)
{
    //performs one-time checks
    getInitSingleton();

    //load the modules for plugin system
    if (not getLoadedModules().empty()) return;
    getLoadedModules() = PluginLoader::loadModules();
}

/***********************************************************************
 * Actual init implementation
 **********************************************************************/
Pothos::InitSingleton::InitSingleton(void)
{
    //check the root
    const Poco::File rootPath(System::getRootPath());
    if (not rootPath.exists() or not rootPath.isDirectory()) throw Exception(
        "Pothos::init()", Poco::format(
        "Pothos installation FAIL - \"%s\" does not exist",
        System::getRootPath()
    ));

    //check for the util executable
    const Poco::File utilExecutablePath(System::getPothosUtilExecutablePath());
    if (not utilExecutablePath.exists()) throw Exception(
        "Pothos::init()", Poco::format(
        "Pothos installation FAIL - \"%s\" does not exist",
        System::getPothosUtilExecutablePath()
    ));
    if (not utilExecutablePath.canExecute()) throw Exception(
        "Pothos::init()", Poco::format(
        "Pothos installation FAIL - \"%s\" not executable",
        System::getPothosUtilExecutablePath()
    ));

    //check for the runtime library
    const Poco::File runtimeLibraryPath(System::getPothosRuntimeLibraryPath());
    if (not runtimeLibraryPath.exists() or not runtimeLibraryPath.isFile()) throw Exception(
        "Pothos::init()", Poco::format(
        "Pothos installation FAIL - \"%s\" does not exist",
        System::getPothosRuntimeLibraryPath()
    ));

    //check the user data directory
    Poco::File userDataPath(System::getUserDataPath());
    if (not userDataPath.exists()) userDataPath.createDirectories();
    if (not userDataPath.exists() or not userDataPath.canWrite()) throw Exception(
        "Pothos::init()", Poco::format(
        "Pothos user directory FAIL - \"%s\" permissions error",
        System::getUserDataPath()
    ));

    //check the user config directory
    Poco::File userConfigPath(System::getUserConfigPath());
    if (not userConfigPath.exists()) userConfigPath.createDirectories();
    if (not userConfigPath.exists() or not userConfigPath.canWrite()) throw Exception(
        "Pothos::init()", Poco::format(
        "Pothos user directory FAIL - \"%s\" permissions error",
        System::getUserConfigPath()
    ));

    //check the development environment (includes)
    Poco::File devIncludePath(System::getPothosDevIncludePath());
    if (not devIncludePath.exists()) throw Exception(
        "Pothos::init()", Poco::format(
        "Pothos development include directory FAIL - \"%s\" does not exist",
        System::getPothosDevIncludePath()
    ));

    //check the development environment (libraries)
    Poco::File devLibraryPath(System::getPothosDevLibraryPath());
    if (not devLibraryPath.exists()) throw Exception(
        "Pothos::init()", Poco::format(
        "Pothos development library directory FAIL - \"%s\" does not exist",
        System::getPothosDevLibraryPath()
    ));
}

void Pothos::deinit(void)
{
    getLoadedModules().clear();
}

Pothos::ScopedInit::ScopedInit(void)
{
    Pothos::init();
}

Pothos::ScopedInit::~ScopedInit(void)
{
    Pothos::deinit();
}
