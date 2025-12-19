#pragma once

#include <string>
#include <vector>

#ifdef ENABLE_SSH
#include <libssh/libssh.h>
#endif

namespace tbox {

    class SshManager {
    public:
        SshManager();
        ~SshManager();

        bool connect(const std::string& host, int port, const std::string& user, const std::string& password);
        void disconnect();
        bool isConnected() const;
        std::string executeCommand(const std::string& command);
        bool startReverseTunnel(int remotePort, const std::string& localHost, int localPort);

    private:
#ifdef ENABLE_SSH
        ssh_session my_ssh_session;
#endif
        bool connected;
    };

}