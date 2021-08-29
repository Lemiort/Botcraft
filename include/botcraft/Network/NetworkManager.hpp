#pragma once

#include "protocolCraft/Handler.hpp"
#include "protocolCraft/enums.hpp"

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace Botcraft
{
	class TCP_Com;
	class Authentifier;

	class NetworkManager : public ProtocolCraft::Handler
	{
	public:
		NetworkManager(const std::string& address, const std::string& login, const std::string& password, const std::string& launcher_accounts_path);
		// Used to create a dummy network manager that does not fire any message
		// but is always in constant_connection_state
		NetworkManager(const ProtocolCraft::ConnectionState constant_connection_state);
		~NetworkManager();

		void AddHandler(ProtocolCraft::Handler* h);
		void Send(const std::shared_ptr<ProtocolCraft::Message> msg);
		const ProtocolCraft::ConnectionState GetConnectionState() const;
		const std::string& GetMyName() const;

	private:
		void WaitForNewPackets();
		void ProcessPacket(const std::vector<unsigned char>& packet);
		void OnNewRawData(const std::vector<unsigned char>& packet);


		virtual void Handle(ProtocolCraft::Message& msg) override;
		virtual void Handle(ProtocolCraft::ClientboundLoginCompressionPacket& msg) override;
		virtual void Handle(ProtocolCraft::ClientboundGameProfilePacket& msg) override;
		virtual void Handle(ProtocolCraft::ClientboundHelloPacket& msg) override;
		virtual void Handle(ProtocolCraft::ClientboundKeepAlivePacket& msg) override;

	private:
		std::vector<ProtocolCraft::Handler*> subscribed;

		std::shared_ptr<TCP_Com> com;
		std::shared_ptr<Authentifier> authentifier;
		ProtocolCraft::ConnectionState state;

		std::thread m_thread_process;//Thread running to process incoming packets without blocking com

		std::queue<std::vector<unsigned char> > packets_to_process;
		std::mutex mutex_process;
		std::condition_variable process_condition;
		int compression;

		std::mutex mutex_send;

		std::string name;

	};
}