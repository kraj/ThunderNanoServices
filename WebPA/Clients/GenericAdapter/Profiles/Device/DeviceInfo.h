#pragma once

#include <glib.h>
#include <interfaces/json/JsonData_DeviceInfo.h>
#include <proc/readproc.h>

#include "ProfileUtils.h"
#include "WebPADataTypes.h"

namespace WPEFramework {

template<class T>
struct FuncPtr {
    typedef FaultCode (T::*GetFunc)(Data&, bool&) const;
    typedef FaultCode (T::*SetFunc)(const Data&);
};

class DeviceInfo {
private:
    static constexpr const TCHAR* ProcessPrefix = _T("Device.DeviceInfo.ProcessStatus.Process.");
    static constexpr const TCHAR* StateRunning = _T("Running");
    static constexpr const TCHAR* StateSleeping = _T("Sleeping");
    static constexpr const TCHAR* StateStopped = _T("Stopped");
    static constexpr const TCHAR* StateZombie = _T("Zombie");

    typedef std::map<std::string, std::pair<FuncPtr<DeviceInfo>::GetFunc, FuncPtr<DeviceInfo>::SetFunc>> FunctionMap;

private:
    class Process {
    private:
    typedef std::map<std::string, std::pair<FuncPtr<DeviceInfo::Process>::GetFunc, FuncPtr<DeviceInfo::Process>::SetFunc>> FunctionMap;

    public:
        Process(const Process&) = delete;
        Process& operator=(const Process&) = delete;
    public:
        Process(uint32_t id);
        virtual ~Process();

        static Process* Instance(uint32_t id);
        static void CloseInstance(Process *);
        static GList* Instances();
        static void CloseInstances();

        FaultCode Parameter(const std::string& name, Data& parameter, bool& changed) const;

    private:
        FaultCode Pid(Data& parameter, bool& changed) const;
        FaultCode Command(Data& parameter, bool& changed) const;
        FaultCode Size(Data& parameter, bool& changed) const;
        FaultCode Priority(Data& parameter, bool& changed) const;
        FaultCode CPUTime(Data& parameter, bool& changed) const;
        FaultCode State(Data& parameter, bool& changed) const;

        bool ProcessFields(proc_t& procTask) const;

    private:
        uint32_t _id;

        // Process values to be saved to check changes happened later
        mutable unsigned int _pid;
        mutable unsigned int _priority;
        mutable unsigned int _cpuTime;
        mutable unsigned int _size;
        mutable std::string _command;
        mutable std::string _state;

        static GHashTable* _hashTable;
        FunctionMap _functionMap;
    };

public:
    DeviceInfo(const DeviceInfo&) = delete;
    DeviceInfo& operator=(const DeviceInfo&) = delete;

    static DeviceInfo* Instance();

public:
    DeviceInfo();
    virtual ~DeviceInfo();

    FaultCode Parameter(const std::string& name, Data& parameter, bool& changed) const;
    FaultCode Parameter(const std::string& name, const Data& parameter);

private:
    void Info();
    FaultCode Manufacturer(Data& parameter, bool& changed) const;
    FaultCode ModelName(Data& parameter, bool& changed) const;
    FaultCode Description(Data& parameter, bool& changed) const;
    FaultCode Description(const Data& parameter);
    FaultCode ProductClass(Data& parameter, bool& changed) const;
    FaultCode SerialNumber(Data& parameter, bool& changed) const;
    FaultCode HardwareVersion(Data& parameter, bool& changed) const;
    FaultCode SoftwareVersion(Data& parameter, bool& changed) const;
    FaultCode AdditionalHardwareVersion(Data& parameter, bool& changed) const;
    FaultCode AdditionalSoftwareVersion(Data& parameter, bool& changed) const;
    FaultCode UpTime(Data& parameter, bool& changed) const;
    FaultCode ProcessorNumberOfEntries(Data& parameter, bool& changed) const;
    FaultCode TotalMemory(Data& parameter, bool& changed) const;
    FaultCode FreeMemory(Data& parameter, bool& changed) const;
    FaultCode CPUUsage(Data& parameter, bool& changed) const;
    FaultCode ProcessNumberOfEntries(Data& parameter, bool& changed) const;
    FaultCode ProcessParameter(Data& parameter, bool& changed) const;

private:
    FunctionMap _functionMap;

    JsonData::DeviceInfo::SysteminfoData _systemInfoData;
};

}
