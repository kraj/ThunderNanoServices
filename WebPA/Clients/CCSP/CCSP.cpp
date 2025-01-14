#include <core/core.h>
#include <tracing/tracing.h>

#include <signal.h>
#include <string>
#include <interfaces/IWebPA.h>

#ifdef __cplusplus
extern "C"
{
#endif
int Parodus2CCSPMain();
#ifdef __cplusplus
}
#endif

namespace WPEFramework {

namespace Client {

    static constexpr char CCSPConfigLink[] = "/tmp/ccsp_msg.cfg";

class CCSP :
    public Exchange::IWebPA::IWebPAClient, public WPEFramework::Core::Thread {

private:
    CCSP(const CCSP&) = delete;
    CCSP& operator=(const CCSP&) = delete;

    class Config : public Core::JSON::Container {
    private:
        Config(const Config&);
        Config& operator=(const Config&);

    public:
        Config()
            : Core::JSON::Container()
            , DataFile(_T(""))
            , ClientURL(_T("tcp://127.0.0.1:6667"))
            , ParodusURL(_T("tcp://127.0.0.1:6666"))
            , CCSPConfigFile(_T("/usr/share/ccspcommonlibrary/ccsp_msg.cfg"))
        {
            Add(_T("datafile"), &DataFile);
            Add(_T("clienturl"), &ClientURL);
            Add(_T("paroduslocalurl"), &ParodusURL);
            Add(_T("ccspconfigfile"), &CCSPConfigFile);
        }
        ~Config()
        {
        }

    public:
        Core::JSON::String DataFile;
        Core::JSON::String ClientURL;
        Core::JSON::String ParodusURL;
        Core::JSON::String CCSPConfigFile;
    };

public:
    CCSP ()
        : Core::Thread(0, _T("WebPAClient"))
        , _adminLock()
    {
        TRACE(Trace::Information, (_T("CCSP::Construct()")));
    }

    virtual ~CCSP()
    {
        TRACE(Trace::Information, (_T("CCSP::Destruct()")));

        if (true == IsRunning()) {
            //Handle the exit sequence here
            Stop();
            Wait(Thread::STOPPED, Core::infinite);
        }
    }

    BEGIN_INTERFACE_MAP(CCSP)
       INTERFACE_ENTRY(Exchange::IWebPA::IWebPAClient)
    END_INTERFACE_MAP

    //WebPAClient Interface
    virtual uint32_t Configure(PluginHost::IShell* service) override
    {
        ASSERT(nullptr != service);
        Config config;
        config.FromString(service->ConfigLine());
        TRACE_GLOBAL(Trace::Information, (_T("DataFile = [%s] ParodusURL = [%s] ClientURL = [%s]"), config.DataFile.Value().c_str(), config.ParodusURL.Value().c_str(), config.ClientURL.Value().c_str()));
        if (config.DataFile.Value().empty() != true) {
            Core::SystemInfo::SetEnvironment(_T("DATA_FILE"), config.DataFile.Value().c_str());
        } else {
            // Set default url for parodus and clients
            Core::SystemInfo::SetEnvironment(_T("PARODUS_URL"), config.ParodusURL.Value().c_str());
            Core::SystemInfo::SetEnvironment(_T("PARODUS2CCSP_CLIENT_URL"), config.ClientURL.Value().c_str());
        }

        Core::File configFile(config.CCSPConfigFile.Value());
        configFile.Link(CCSPConfigLink);

        return (Core::ERROR_NONE);
    }

    virtual void Launch() override
    {
        Block();
        Wait(Thread::BLOCKED | Thread::STOPPED, Core::infinite);
        if (State() == Thread::BLOCKED) {

                // Call Parodus listner worker function
                Run();
        }
    }

private:
    virtual uint32_t Worker();

private:
    Core::CriticalSection _adminLock;
};

// The essence of making the IWebPAClient interface available. This instantiates
// an object that can be created from the outside of the library by looking
// for the CCSP class name, that realizes the IStateControl interface.
SERVICE_REGISTRATION(CCSP, 1, 0);

uint32_t CCSP::Worker()
{
    TRACE(Trace::Information, (string(__FUNCTION__)));

    Parodus2CCSPMain();

    TRACE(Trace::Information, (_T("End of Worker\n")));
    return WPEFramework::Core::infinite;
}

} // Client
} //WPEFramework
