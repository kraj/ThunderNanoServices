#include "Module.h"
#include "DeviceInfo.h"

#include <interfaces/json/JsonData_DeviceInfo.h>
#include <jsonrpc/jsonrpc.h>

namespace WPEFramework {

GHashTable* DeviceInfo::Process::_hashTable = nullptr;

DeviceInfo::Process::Process(uint32_t id)
    : _id(id)
    , _pid(0)
    , _priority(0)
    , _cpuTime(0)
    , _size(0)
    , _command()
    , _state()
{
    _functionMap.insert(std::make_pair("PID", std::make_pair(&DeviceInfo::Process::Pid, nullptr)));
    _functionMap.insert(std::make_pair("Command", std::make_pair(&DeviceInfo::Process::Command, nullptr)));
    _functionMap.insert(std::make_pair("Size", std::make_pair(&DeviceInfo::Process::Size, nullptr)));
    _functionMap.insert(std::make_pair("Priority", std::make_pair(&DeviceInfo::Process::Priority, nullptr)));
    _functionMap.insert(std::make_pair("CPUTime", std::make_pair(&DeviceInfo::Process::CPUTime, nullptr)));
    _functionMap.insert(std::make_pair("State", std::make_pair(&DeviceInfo::Process::State, nullptr)));
}

DeviceInfo::Process::~Process()
{
    _functionMap.clear();
}

DeviceInfo::Process* DeviceInfo::Process::Instance(uint32_t id)
{
    TRACE_GLOBAL(Trace::Information, (string(__FUNCTION__)));
    DeviceInfo::Process* pRet = nullptr;

    if (_hashTable) {
        pRet = (DeviceInfo::Process *)g_hash_table_lookup(_hashTable, (gpointer) id);
    } else {
        _hashTable = g_hash_table_new(nullptr, nullptr);
    }

    if (!pRet) {
        pRet = new DeviceInfo::Process(id);
        g_hash_table_insert(_hashTable, (gpointer)id, pRet);
    }
    return pRet;
}

void DeviceInfo::Process::CloseInstance(DeviceInfo::Process* process)
{
    TRACE_GLOBAL(Trace::Information, (string(__FUNCTION__)));
    if (process) {
        g_hash_table_remove(_hashTable, (gconstpointer)process->_id);
        delete process;
    }
}

GList* DeviceInfo::Process::Instances()
{
    TRACE_GLOBAL(Trace::Information, (string(__FUNCTION__)));
    if (_hashTable)
        return g_hash_table_get_keys(_hashTable);
    return nullptr;
}

void DeviceInfo::Process::CloseInstances()
{
    TRACE_GLOBAL(Trace::Information, (string(__FUNCTION__)));
    if (_hashTable) {
        GList* tmpList = g_hash_table_get_values (_hashTable);

        while (tmpList) {
            DeviceInfo::Process* process = (DeviceInfo::Process *)tmpList->data;
            tmpList = tmpList->next;
            CloseInstance(process);
        }
    }
}

bool DeviceInfo::Process::ProcessFields(proc_t& procTask) const
{
    PROCTAB* procTab = nullptr;
    uint32_t procEntry = 0;
    bool status = false;

    if ((procTab = openproc(PROC_FILLSTAT | PROC_FILLMEM)) != nullptr) {
        for (procEntry = 0; procEntry < _id; procEntry++) {
            memset(&procTask, 0, sizeof(procTask));

            if (!procTab->finder(procTab, &procTask)) {
                TRACE(Trace::Error, (_T("ProcessInstance: %d : No Entry Found In Process Profile Table\n"), procEntry));
                break;
            }
        }
        if (procTab->reader(procTab, &procTask) != nullptr) {
            status = true;
        }
    } else {
        TRACE(Trace::Error, (_T("[%s:%d] Failed in openproc(), returned NULL. \n"), __func__, __LINE__));
    }

    closeproc(procTab);
    return status;
}

FaultCode DeviceInfo::Process::Pid(Data& parameter, bool& changed) const
{
    FaultCode status = NoFault;

    proc_t procTask;
    if (ProcessFields(procTask) == true) {
        unsigned int pid = static_cast<unsigned int>(procTask.tid);
        if (pid != _pid) {
            changed = true;
            _pid = pid;
        }
        parameter.Value(pid);
    } else {
        status = Error;
    }

    return status;
}

FaultCode DeviceInfo::Process::Command(Data& parameter, bool& changed) const
{
    FaultCode status = NoFault;

    proc_t procTask;
    if (ProcessFields(procTask) == true) {
        std::string command(procTask.cmd, strlen(procTask.cmd));
        if (command != _command) {
            changed = true;
            _command = command;
        }
        parameter.Value(command);
    } else {
        status = Error;
    }

    return status;
}

FaultCode DeviceInfo::Process::Size(Data& parameter, bool& changed) const
{
    FaultCode status = NoFault;

    proc_t procTask;
    if (ProcessFields(procTask) == true) {
        unsigned int size = static_cast<unsigned int>(procTask.size * 4);
        if (size != _size) {
            changed = true;
            _size = size;
        }
        parameter.Value(size);
    } else {
        status = Error;
    }

    return status;
}

FaultCode DeviceInfo::Process::Priority(Data& parameter, bool& changed) const
{
    FaultCode status = NoFault;

    proc_t procTask;
    if (ProcessFields(procTask) == true) {
        unsigned int priority = static_cast<unsigned int>(procTask.priority * 4);
        if (priority != _priority) {
            changed = true;
            _priority = priority;
        }
        parameter.Value(priority);
    } else {
        status = Error;
    }

    return status;
}

FaultCode DeviceInfo::Process::CPUTime(Data& parameter, bool& changed) const
{
    FaultCode status = NoFault;

    proc_t procTask;
    if (ProcessFields(procTask) == true) {
        unsigned int cpuTime = static_cast<unsigned int>(procTask.utime + procTask.stime);
        if (cpuTime != _cpuTime) {
            changed = true;
            _cpuTime = cpuTime;
        }
        parameter.Value(cpuTime);
    } else {
        status = Error;
    }

    return status;
}

FaultCode DeviceInfo::Process::State(Data& parameter, bool& changed) const
{
    FaultCode status = NoFault;

    proc_t procTask;
    if (ProcessFields(procTask) == true) {
        std::string state;
        switch (procTask.state) {
        case 'R':
            state = StateRunning;
            break;
        case 'S':
            state = StateSleeping;
            break;
        case 'T':
            state = StateStopped;
            break;
        case 'Z':
            state = StateZombie;
            break;
        default:
            break;
        }
        if (state != _state) {
            changed = true;
            _state = state;
        }
        parameter.Value(state);
    } else {
        status = Error;
    }

    return status;
}

FaultCode DeviceInfo::Process::Parameter(const std::string& name, Data& parameter, bool& changed) const
{
    FaultCode status = MethodNotSupported;

    FunctionMap::const_iterator index = _functionMap.find(name);
    if (index != _functionMap.end()) {
        FuncPtr<DeviceInfo::Process>::GetFunc func = index->second.first;
        if (nullptr != func) {
            status = (this->*func)(parameter, changed);
        }
    }

    return status;
}

DeviceInfo::DeviceInfo()
    : _systemInfoData()
{
    TRACE(Trace::Information, (string(__FUNCTION__)));
    _functionMap.insert(std::make_pair("Manufacturer", std::make_pair(&DeviceInfo::Manufacturer, nullptr)));
    _functionMap.insert(std::make_pair("ModelName", std::make_pair(&DeviceInfo::ModelName, nullptr)));
    _functionMap.insert(std::make_pair("Description", std::make_pair(static_cast<FuncPtr<DeviceInfo>::GetFunc>(&DeviceInfo::Description),
                                                                     static_cast<FuncPtr<DeviceInfo>::SetFunc>(&DeviceInfo::Description))));
    _functionMap.insert(std::make_pair("ProductClass", std::make_pair(&DeviceInfo::ProductClass, nullptr)));
    _functionMap.insert(std::make_pair("SerialNumber", std::make_pair(&DeviceInfo::SerialNumber, nullptr)));
    _functionMap.insert(std::make_pair("HardwareVersion", std::make_pair(&DeviceInfo::HardwareVersion, nullptr)));
    _functionMap.insert(std::make_pair("SoftwareVersion", std::make_pair(&DeviceInfo::SoftwareVersion, nullptr)));
    _functionMap.insert(std::make_pair("AdditionalHardwareVersion", std::make_pair(&DeviceInfo::AdditionalHardwareVersion, nullptr)));
    _functionMap.insert(std::make_pair("AdditionalSoftwareVersion", std::make_pair(&DeviceInfo::AdditionalSoftwareVersion, nullptr)));
    _functionMap.insert(std::make_pair("UpTime", std::make_pair(&DeviceInfo::UpTime, nullptr)));
    _functionMap.insert(std::make_pair("ProcessorNumberOfEntries", std::make_pair(&DeviceInfo::ProcessorNumberOfEntries, nullptr)));
    _functionMap.insert(std::make_pair("Total", std::make_pair(&DeviceInfo::TotalMemory, nullptr)));
    _functionMap.insert(std::make_pair("Free", std::make_pair(&DeviceInfo::FreeMemory, nullptr)));
    _functionMap.insert(std::make_pair("CPUUsage", std::make_pair(&DeviceInfo::CPUUsage, nullptr)));
    _functionMap.insert(std::make_pair("ProcessNumberOfEntries", std::make_pair(&DeviceInfo::ProcessNumberOfEntries, nullptr)));
    _functionMap.insert(std::make_pair("Process", std::make_pair(&DeviceInfo::ProcessParameter, nullptr)));
    Info();
}

DeviceInfo::~DeviceInfo()
{
    TRACE(Trace::Information, (string(__FUNCTION__)));
    _functionMap.clear();
}

DeviceInfo* DeviceInfo::Instance()
{
    static DeviceInfo deviceInfo;
    return &deviceInfo;
}

void DeviceInfo::Info()
{
    Core::SystemInfo::SetEnvironment(_T("THUNDER_ACCESS"), (_T("127.0.0.1:80")));
    JSONRPC::Client remoteObject(_T("DeviceInfo.1"), 1.0);
    if (remoteObject.Get<JsonData::DeviceInfo::SysteminfoData>(1000, _T("systeminfo"), _systemInfoData) == Core::ERROR_NONE) {
        TRACE(Trace::Information, (_T("DeviceInfo: SoftwareVerison = %s, Serialnumber = %s \n"), _systemInfoData.Version.Value().c_str(), _systemInfoData.Serialnumber.Value().c_str()));
    } else {
        TRACE(Trace::Information, (_T("Error in fetching device information through jsonrpc")));
    }
}

FaultCode DeviceInfo::SerialNumber(Data& parameter, bool& changed) const
{
    TRACE(Trace::Information, (string(__FUNCTION__)));
    FaultCode status = NoFault;

    if (_systemInfoData.Serialnumber.Value().empty() != true) {
        parameter.Value(_systemInfoData.Serialnumber.Value());
    } else {
        status = InvalidParameterValue;
    }
    return status;
}

FaultCode DeviceInfo::Manufacturer(Data& parameter, bool& changed) const
{
    TRACE(Trace::Information, (string(__FUNCTION__)));
    FaultCode status = NoFault;
    std::string manufacturer("RPITest");
    parameter.Value(manufacturer);
    return status;
}

FaultCode DeviceInfo::ModelName(Data& parameter, bool& changed) const
{
    TRACE(Trace::Information, (string(__FUNCTION__)));
    FaultCode status = NoFault;
    std::string modelName("RPI3");
    parameter.Value(modelName);

    return status;
}

FaultCode DeviceInfo::Description(Data& parameter, bool& changed) const
{
    TRACE(Trace::Information, (string(__FUNCTION__)));
    FaultCode status = NoFault;
    std::string description("TR-181, TR-135 Datamodel Configuration");
    parameter.Value(description);

    return status;
}

FaultCode DeviceInfo::Description(const Data& parameter)
{
    TRACE(Trace::Information, (string(__FUNCTION__)));
    FaultCode status = NoFault;

    TRACE(Trace::Information, (_T("Description : %s\n"), parameter.Value().String().c_str()));
    return status;
}

FaultCode DeviceInfo::ProductClass(Data& parameter, bool& changed) const
{
    TRACE(Trace::Information, (string(__FUNCTION__)));
    FaultCode status = NoFault;
    std::string productClass("RPI Test");
    parameter.Value(productClass);

    return status;
}

FaultCode DeviceInfo::HardwareVersion(Data& parameter, bool& changed) const
{
    TRACE(Trace::Information, (string(__FUNCTION__)));
    FaultCode status = NoFault;
    std::string hardwareVersion("BCM1.0");
    parameter.Value(hardwareVersion);

    return status;
}

FaultCode DeviceInfo::SoftwareVersion(Data& parameter, bool& changed) const
{
    TRACE(Trace::Information, (string(__FUNCTION__)));
    FaultCode status = NoFault;
    if (_systemInfoData.Version.Value().empty() != true) {
        parameter.Value(_systemInfoData.Version.Value());
    } else {
        status = InvalidParameterValue;
    }
    return status;
}

FaultCode DeviceInfo::AdditionalHardwareVersion(Data& parameter, bool& changed) const
{
    TRACE(Trace::Information, (string(__FUNCTION__)));
    FaultCode status = NoFault;
    std::string additionalHardwareVersion("ADD HW1.1");
    parameter.Value(additionalHardwareVersion);
    return status;
}

FaultCode DeviceInfo::AdditionalSoftwareVersion(Data& parameter, bool& changed) const
{
    TRACE(Trace::Information, (string(__FUNCTION__)));
    FaultCode status = NoFault;
    std::string additionalSoftwareVersion("ADD SW1.1");
    parameter.Value(additionalSoftwareVersion);

    return status;
}

FaultCode DeviceInfo::UpTime(Data& parameter, bool& changed) const
{
    TRACE(Trace::Information, (string(__FUNCTION__)));
    FaultCode status = NoFault;
    if (_systemInfoData.Uptime.Value() != 0) {
        parameter.Value(_systemInfoData.Uptime.Value());
    } else {
        status = InvalidParameterValue;
    }

    return status;
}

FaultCode DeviceInfo::ProcessorNumberOfEntries(Data& parameter, bool& changed) const
{
    TRACE(Trace::Information, (string(__FUNCTION__)));
    FaultCode status = NoFault;
    int numberOfEntries= 0; // To be implemented
    parameter.Value(numberOfEntries);
    return status;
}

FaultCode DeviceInfo::TotalMemory(Data& parameter, bool& changed) const
{
    TRACE(Trace::Information, (string(__FUNCTION__)));
    FaultCode status = NoFault;
    if (_systemInfoData.Totalram.Value() != 0) {
        parameter.Value(_systemInfoData.Totalram.Value());
    } else {
        status = InvalidParameterValue;
    }

    return status;
}

FaultCode DeviceInfo::FreeMemory(Data& parameter, bool& changed) const
{
    TRACE(Trace::Information, (string(__FUNCTION__)));
    FaultCode status = NoFault;
    if (_systemInfoData.Freeram.Value() != 0) {
        parameter.Value(_systemInfoData.Freeram.Value());
    } else {
        status = InvalidParameterValue;
    }

    return status;
}

FaultCode DeviceInfo::CPUUsage(Data& parameter, bool& changed) const
{
    TRACE(Trace::Information, (string(__FUNCTION__)));
    FaultCode status = NoFault;
    if (_systemInfoData.Cpuload.Value().empty() != true) {
        parameter.Value(_systemInfoData.Cpuload.Value());
    } else {
        status = InvalidParameterValue;
    }

    return status;
}

FaultCode DeviceInfo::ProcessNumberOfEntries(Data& parameter, bool& changed) const
{
    TRACE(Trace::Information, (string(__FUNCTION__)));
    FaultCode status = NoFault;

    PROCTAB* procTab = nullptr;
    proc_t* process = nullptr;
    int numberOfEntries = 0;

    if ((procTab = openproc(PROC_FILLMEM)) != nullptr) {
        while ((process = readproc(procTab, nullptr)) != nullptr) {
            numberOfEntries++;
        }
    } else {
        TRACE(Trace::Error, (_T("[%s:%d] Failed in openproc(), returned NULL. \n"), __func__, __LINE__));
    }

    closeproc(procTab);

    parameter.Value(numberOfEntries);

    return status;
}

FaultCode DeviceInfo::ProcessParameter(Data& parameter, bool& changed) const
{
    TRACE(Trace::Information, (string(__FUNCTION__)));
    FaultCode status = NoFault;
    string name;
    uint32_t id = 0;
    if (Utils::MatchComponent(parameter.Name(), string(ProcessPrefix), name, id)) {
        DeviceInfo::Process* instance = DeviceInfo::Process::Instance(id);
        if (instance) {
            status = instance->Parameter(name, parameter, changed);
        }
    } else {
        status = FaultCode::InvalidParameterName;
    }

    return status;
}

FaultCode DeviceInfo::Parameter(const std::string& name, Data& parameter, bool& changed) const
{
    FaultCode status = MethodNotSupported;

    FunctionMap::const_iterator index = _functionMap.find(name);
    if (index != _functionMap.end()) {
        FuncPtr<DeviceInfo>::GetFunc func = index->second.first;
        if (nullptr != func) {
            status = (this->*func)(parameter, changed);
        }
    }

    return status;
}

FaultCode DeviceInfo::Parameter(const std::string& name, const Data& parameter)
{
    FaultCode status = MethodNotSupported;

    FunctionMap::iterator index = _functionMap.find(name);
    if (index != _functionMap.end()) {
        FuncPtr<DeviceInfo>::SetFunc func = index->second.second;
        if (nullptr != func) {
            status = (this->*func)(parameter);
        }
    }
    return status;
}

}
