#include "TestUtility.h"

namespace WPEFramework {
namespace TestUtility {
    Exchange::IMemory* MemoryObserver(const RPC::IRemoteConnection* connection)
    {
        class MemoryObserverImpl : public Exchange::IMemory {
        private:
            MemoryObserverImpl();
            MemoryObserverImpl(const MemoryObserverImpl&);
            MemoryObserverImpl& operator=(const MemoryObserverImpl&);

        public:
            MemoryObserverImpl(const RPC::IRemoteConnection* connection)
                : _main(connection  == nullptr ? Core::ProcessInfo().Id() : connection->RemoteId())
                , _observable(false)
            {

            }
            ~MemoryObserverImpl() = default;
        public:
            virtual void Observe(const uint32_t pid)
            {
                if (pid == 0) {
                    _observable = false;
                } else {
                    _main = Core::ProcessInfo(pid);
                    _observable = true;
                }
            }
            virtual uint64_t Resident() const { return (_observable == false ? 0 : _main.Resident()); }

            virtual uint64_t Allocated() const { return (_observable == false ? 0 : _main.Allocated()); }

            virtual uint64_t Shared() const { return (_observable == false ? 0 : _main.Shared()); }

            virtual uint8_t Processes() const { return (IsOperational() ? 1 : 0); }

            virtual const bool IsOperational() const { return (_observable == false) || (_main.IsActive()); }

            BEGIN_INTERFACE_MAP(MemoryObserverImpl)
            INTERFACE_ENTRY(Exchange::IMemory)
            END_INTERFACE_MAP

        private:
            Core::ProcessInfo _main;
            bool _observable;
        };

        Exchange::IMemory* memory_observer = (Core::Service<MemoryObserverImpl>::Create<Exchange::IMemory>(connection));

        return memory_observer;
    }
} // namespace TestUtility

namespace Plugin {
    SERVICE_REGISTRATION(TestUtility, 1, 0);

    /* virtual */ const string TestUtility::Initialize(PluginHost::IShell* service)
    {
        string message = EMPTY_STRING;
        Config config;

        ASSERT(service != nullptr);
        ASSERT(_service == nullptr);
        ASSERT(_testUtilityImp == nullptr);
        ASSERT(_memory == nullptr);
        ASSERT(_connection == 0);

        _service = service;
        _skipURL = static_cast<uint8_t>(_service->WebPrefix().length());
        
        config.FromString(_service->ConfigLine());
        
        _service->Register(&_notification);
        _testUtilityImp = _service->Root<Exchange::ITestUtility>(_connection, ImplWaitTime, _T("TestUtilityImp"));

        if (_testUtilityImp != nullptr) {
            RPC::IRemoteConnection* remoteConnection = _service->RemoteConnection(_connection);
            if (remoteConnection) {
                _memory = WPEFramework::TestUtility::MemoryObserver(remoteConnection);
                _memory->Observe(remoteConnection->RemoteId());
                remoteConnection->Release();
            } else {
                _memory = nullptr;
                TRACE(Trace::Warning, (_T("Colud not create MemoryObserver in TestUtility")));
            }
        } else {
            ProcessTermination(_connection);

            _service->Unregister(&_notification);
            _service = nullptr;

            TRACE(Trace::Fatal, (_T("*** TestUtility could not be instantiated ***")))
            message = _T("TestUtility could not be instantiated.");
        }

        return message;
    }

    /* virtual */ void TestUtility::Deinitialize(PluginHost::IShell* service)
    {
        ASSERT(_service == service);
        ASSERT(_testUtilityImp != nullptr);
        ASSERT(_memory != nullptr);

        _service->Unregister(&_notification);
        _service = nullptr;
        if (_memory->Release() != Core::ERROR_DESTRUCTION_SUCCEEDED) {
            TRACE(Trace::Information, (_T("Memory observer in TestUtility is not properly destructed")));
        }
        _memory = nullptr;
        
        if (_testUtilityImp->Release() != Core::ERROR_DESTRUCTION_SUCCEEDED) {
            TRACE(Trace::Information, (_T("TestUtility is not properly destructed (connection=%d)"), _connection));
            ProcessTermination(_connection);
        }

        _testUtilityImp = nullptr;
        _skipURL = 0;
        _connection = 0;
    }

    /* virtual */ string TestUtility::Information() const
    {
        // No additional info to report.
        return ((_T("The purpose of this plugin is provide ability to execute functional tests.")));
    }

    static Core::ProxyPoolType<Web::TextBody> _testUtilityMetadata(2);

    /* virtual */ void TestUtility::Inbound(Web::Request& request)
    {
        if ((request.Verb == Web::Request::HTTP_POST) || (request.Verb == Web::Request::HTTP_PUT)) {
            request.Body(_testUtilityMetadata.Element());
        }
    }

    /* virtual */ Core::ProxyType<Web::Response> TestUtility::Process(const Web::Request& request)
    {
        ASSERT(_skipURL <= request.Path.length());
        Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());

        if (_testUtilityImp != nullptr) {
            Core::ProxyType<Web::TextBody> body(_testUtilityMetadata.Element());
            string requestBody = EMPTY_STRING;

            if (((request.Verb == Web::Request::HTTP_POST) || (request.Verb == Web::Request::HTTP_PUT)) && (request.HasBody())) {
                requestBody = (*request.Body<Web::TextBody>());
            }

            (*body) = HandleRequest(request.Verb, request.Path, _skipURL, requestBody);
            if ((*body) != EMPTY_STRING) {
                result->Body<Web::TextBody>(body);
                result->ErrorCode = Web::STATUS_OK;
                result->Message = (_T("OK"));
                result->ContentType = Web::MIMETypes::MIME_JSON;
            } else {
                result->ErrorCode = Web::STATUS_BAD_REQUEST;
                result->Message = (_T("Method is not supported"));
            }
        } else {
            result->ErrorCode = Web::STATUS_METHOD_NOT_ALLOWED;
            result->Message = (_T("Test controller does not exist"));
        }
        return result;
    }

    void TestUtility::ProcessTermination(uint32_t _connection)
    {
        RPC::IRemoteConnection* process(_service->RemoteConnection(_connection));
        if (process != nullptr) {
            process->Terminate();
            process->Release();
        }
    }

    void TestUtility::Deactivated(RPC::IRemoteConnection* process)
    { 
        if (_connection == process->Id()) {
            ASSERT(_service != nullptr);
            Core::WorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(_service, PluginHost::IShell::DEACTIVATED, PluginHost::IShell::FAILURE));
        }
    }

    string /*JSON*/ TestUtility::TestCommands(void)
    {
        string response = EMPTY_STRING;
        Metadata testCommands;

        Exchange::ITestUtility::ICommand::IIterator* supportedCommands = _testUtilityImp->Commands();
        ASSERT(supportedCommands != nullptr);

        while (supportedCommands->Next()) {
            Core::JSON::String name;
            name = supportedCommands->Command()->Name();
            testCommands.TestCommands.Add(name);
        }
        testCommands.ToString(response);

        return response;
    }

    string /*JSON*/ TestUtility::HandleRequest(Web::Request::type type, const string& path, const uint8_t skipUrl, const string& body /*JSON*/)
    {
        bool executed = false;
        // Return empty result in case of issue
        string /*JSON*/ response = EMPTY_STRING;

        Core::TextSegmentIterator index(Core::TextFragment(path, skipUrl, path.length() - skipUrl), false, '/');
        index.Next();
        index.Next();
        // Here process request other than:
        // /Service/<CALLSIGN>/TestCommands
        // /Service/<CALLSIGN>/<TEST_COMMAND_NAME>/...

        if (index.IsValid()) {
            if (index.Current().Text() == _T("TestCommands")) {
                if ((!index.Next()) && (type == Web::Request::HTTP_GET)) {
                    response = TestCommands();
                    executed = true;
                }
            } else {
                Exchange::ITestUtility::ICommand* command = _testUtilityImp->Command(index.Current().Text());

                if (command) {
                    if (!index.Next() && ((type == Web::Request::HTTP_POST) || (type == Web::Request::HTTP_PUT))) {
                        // Execute test command
                        response = command->Execute(body);
                        executed = true;
                    } else  if (type == Web::Request::HTTP_GET) {
                        if (index.IsValid() && (index.Current().Text() == _T("Description")) && (!index.Next())) {
                            response = command->Description();
                            executed = true;
                        } else if (index.IsValid() && (index.Current().Text() == _T("Parameters")) && (!index.Next())) {
                            response = command->Signature();
                            executed = true;
                        }
                    }
                }
            }
        }

        if (!executed) {
            TRACE(Trace::Fatal, (_T("*** Test case method not found !!! ***")))
        }

        return response;
    }
} // namespace Plugin
} // namespace WPEFramework
