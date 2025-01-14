#include "Module.h"

#include "DeviceInfo.h"
#include "DeviceControl.h"

namespace WPEFramework {

bool DeviceControl::Initialize()
{
   TRACE(Trace::Information, (string(__FUNCTION__)));
   _prefixList.push_back("Device.DeviceInfo.ProcessStatus.");
   _prefixList.push_back("Device.DeviceInfo.MemeoryStatus.");
   _prefixList.push_back("Device.DeviceInfo.");
   return true;
}

bool DeviceControl::Deinitialize()
{
    TRACE(Trace::Information, (string(__FUNCTION__)));
    _notifier.clear();
    _prefixList.clear();
    return true;
}

DeviceControl::DeviceControl()
    : _notifier()
    , _prefixList()
    , _adminLock()
    , _callback(nullptr)
{
}

DeviceControl::~DeviceControl()
{
    TRACE(Trace::Information, (string(__FUNCTION__)));
}

FaultCode DeviceControl::Parameter(Data& parameter) const {
    TRACE(Trace::Information, (string(__FUNCTION__)));

    FaultCode ret = FaultCode::Error;
    uint32_t instance = 0;
    for (auto& prefix : _prefixList) {
        if (parameter.Name().compare(0, prefix.length(), prefix) == 0) {
            std::string name;
            if (Utils::MatchComponent(parameter.Name(), prefix, name, instance)) {
                _adminLock.Lock();
                DeviceInfo* deviceInfo = DeviceInfo::Instance();
                if (deviceInfo) {
                    bool changed;
                    ret = deviceInfo->Parameter(name, parameter, changed);
                }
                _adminLock.Unlock();
                break;
            } else {
                ret = FaultCode::InvalidParameterName;
           }
        }
    }

    return ret;
}

FaultCode DeviceControl::Parameter(const Data& parameter) {
    TRACE(Trace::Information, (string(__FUNCTION__)));

    FaultCode ret = FaultCode::Error;
    uint32_t instance = 0;

    for (auto& prefix : _prefixList) {
        if (parameter.Name().compare(0, prefix.length(), prefix) == 0) {
            std::string name;
            if (Utils::MatchComponent(parameter.Name(), prefix, name, instance)) {
                _adminLock.Lock();
                DeviceInfo* deviceInfo = DeviceInfo::Instance();
                if (deviceInfo) {
                     ret = deviceInfo->Parameter(name, parameter);
                }
                _adminLock.Unlock();
                break;
            } else {
                ret = FaultCode::InvalidParameterName;
            }
        }
    }

    return ret;
}

FaultCode DeviceControl::Attribute(Data& parameter) const {
    TRACE(Trace::Information, (string(__FUNCTION__)));

    FaultCode ret = FaultCode::Error;

    _adminLock.Lock();
    NotifierMap::const_iterator notifier = _notifier.find(parameter.Name());
    if (notifier != _notifier.end()) {
         parameter.Value(notifier->second);
    } else {
        ret = FaultCode::InvalidParameterName;
    }
    _adminLock.Unlock();

    return ret;
}

FaultCode DeviceControl::Attribute(const Data& parameter) {
    TRACE(Trace::Information, (string(__FUNCTION__)));

    FaultCode ret = FaultCode::Error;

    _adminLock.Lock();
    _notifier.insert(std::make_pair(parameter.Name(), true));
    _adminLock.Unlock();

    return ret;
}

void DeviceControl::SetCallback(IProfileControl::ICallback* cb)
{
    _adminLock.Lock();
    _callback = cb;
    _adminLock.Unlock();
}

void DeviceControl::CheckForUpdates()
{
    TRACE_GLOBAL(Trace::Information, (string(__FUNCTION__)));

    for (auto& index : _notifier) {
        if (index.second == true) {
            bool changed = false;
            Data parameter(index.first);
            for (auto& prefix : _prefixList) {
                uint32_t instance = 0;
                std::string name;
                if (Utils::MatchComponent(parameter.Name(), prefix, name, instance)) {
                    _adminLock.Lock();
                    DeviceInfo* deviceInfo = DeviceInfo::Instance();
                    if (deviceInfo) {
                        deviceInfo->Parameter(name, parameter, changed);
                        if (changed == true) {
                            if (_callback) {
                                _callback->NotifyEvent(EVENT_VALUECHANGED, parameter);
                            }
                        }
                    }
                    _adminLock.Unlock();
                }
            }
        }
    }
}

static Administrator::ProfileImplementationType<DeviceControl> Register;

}
