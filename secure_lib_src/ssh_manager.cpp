#include "ssh_manager.hpp"
#include <iostream>
#include <sstream>

namespace tbox {

    SshManager::SshManager() : connected(false) {
#ifdef ENABLE_SSH
        my_ssh_session = ssh_new();
#endif
    }

    SshManager::~SshManager() {
        disconnect();
#ifdef ENABLE_SSH
        if (my_ssh_session) {
            ssh_free(my_ssh_session);
        }
#endif
    }

    bool SshManager::connect(const std::string& host, int port, const std::string& user, const std::string& password) {
#ifdef ENABLE_SSH
        if (!my_ssh_session) {
            my_ssh_session = ssh_new();
        }
        if (!my_ssh_session) return false;

        ssh_options_set(my_ssh_session, SSH_OPTIONS_HOST, host.c_str());
        ssh_options_set(my_ssh_session, SSH_OPTIONS_PORT, &port);
        ssh_options_set(my_ssh_session, SSH_OPTIONS_USER, user.c_str());

        int rc = ssh_connect(my_ssh_session);
        if (rc != SSH_OK) return false;

        rc = ssh_userauth_password(my_ssh_session, nullptr, password.c_str());
        if (rc != SSH_AUTH_SUCCESS) {
            ssh_disconnect(my_ssh_session);
            return false;
        }

        connected = true;
        return true;
#else
        return false;
#endif
    }

    void SshManager::disconnect() {
#ifdef ENABLE_SSH
        if (connected && my_ssh_session) {
            ssh_disconnect(my_ssh_session);
            connected = false;
        }
#endif
    }

    bool SshManager::isConnected() const {
        return connected;
    }

    std::string SshManager::executeCommand(const std::string& command) {
#ifdef ENABLE_SSH
        if (!connected || !my_ssh_session) return "";

        ssh_channel channel = ssh_channel_new(my_ssh_session);
        if (!channel) return "";

        if (ssh_channel_open_session(channel) != SSH_OK) {
            ssh_channel_free(channel);
            return "";
        }

        if (ssh_channel_request_exec(channel, command.c_str()) != SSH_OK) {
            ssh_channel_close(channel);
            ssh_channel_free(channel);
            return "";
        }

        std::stringstream output;
        char buffer[256];
        int nbytes;
        while ((nbytes = ssh_channel_read(channel, buffer, sizeof(buffer), 0)) > 0) {
            output.write(buffer, nbytes);
        }

        ssh_channel_send_eof(channel);
        ssh_channel_close(channel);
        ssh_channel_free(channel);
        return output.str();
#else
        return "SSH Disabled";
#endif
    }

    bool SshManager::startReverseTunnel(int remotePort, const std::string& localHost, int localPort) {
        return false;
    }
}