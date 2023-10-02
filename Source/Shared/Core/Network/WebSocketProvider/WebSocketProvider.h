#pragma once
#include <mutex>
#include <future>
#include <string>
#include <unordered_map>
#include <queue>
#include <thread>

#include "Shared/Core/Utils/Logging.h"

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <websocketpp/client.hpp>

class WebSocketProvider
{
protected:
	WebSocketProvider();
	~WebSocketProvider();
public:
	WebSocketProvider(WebSocketProvider& other) = delete;
	void operator=(const WebSocketProvider&) = delete;
	static WebSocketProvider* GetInstance();

	std::unordered_map<std::string, std::promise<websocketpp::connection_hdl>*> Endpoints;
	std::unordered_map<boost::asio::detail::socket_ops::shared_cancel_token_type, std::queue<std::string>> ReceivedMessages;
	void AddNewEndpoint(std::string);
	websocketpp::connection_hdl Accept(std::string);
	std::string Peek(websocketpp::connection_hdl);
	std::string Read(websocketpp::connection_hdl);
	bool Send(websocketpp::connection_hdl, std::vector<uint8_t>&, size_t&);
	bool Close(websocketpp::connection_hdl);
	//
	websocketpp::server<websocketpp::config::asio>* GetServer();
	



private:
	static WebSocketProvider* singleton_;
	static std::mutex mutex_;
	//
	websocketpp::server<websocketpp::config::asio> server;
	void on_open(websocketpp::connection_hdl);
	void on_message(websocketpp::connection_hdl hdl, websocketpp::connection<websocketpp::config::asio>::message_ptr);

	//
	void StartServer(int);
};
WebSocketProvider* WebSocketProvider::singleton_{ nullptr };
std::mutex WebSocketProvider::mutex_;

WebSocketProvider::WebSocketProvider()
{
	this->StartServer(80);
}

WebSocketProvider::~WebSocketProvider()
{
}

inline WebSocketProvider* WebSocketProvider::GetInstance()
{
	std::lock_guard<std::mutex> lock(mutex_);
	if (WebSocketProvider::singleton_ == nullptr) {
		WebSocketProvider::singleton_ = new WebSocketProvider();
	}
	return WebSocketProvider::singleton_;
}

inline void WebSocketProvider::AddNewEndpoint(std::string name)
{
	auto endpoint = this->Endpoints.find(name);
	if (endpoint == this->Endpoints.end()) {
		Endpoints[name] = new std::promise<websocketpp::connection_hdl>();
	}
}

inline websocketpp::connection_hdl WebSocketProvider::Accept(std::string name)
{
	auto endpoint = this->Endpoints.find(name);
	if (endpoint != this->Endpoints.end()) {
		if (endpoint->second) {
			auto promice_ptr = endpoint->second;
			auto future = promice_ptr->get_future();
			Log("Waiting For Future For: %s", name.c_str());
			//future.wait();
			auto hdl = future.get();
			Log("Got Future For: %s", name.c_str());
			delete promice_ptr;
			Endpoints[name] = new std::promise<websocketpp::connection_hdl>();
			return hdl;
		}
	}
	return websocketpp::connection_hdl();
}

inline std::string WebSocketProvider::Peek(websocketpp::connection_hdl hdl)
{
	auto connection = this->ReceivedMessages.find(hdl.lock());
	if (connection != this->ReceivedMessages.end()) {
		if (!connection->second.empty()) {
			return connection->second.front();
		}
	}
	return std::string();
}

inline std::string WebSocketProvider::Read(websocketpp::connection_hdl hdl)
{
	auto connection = this->ReceivedMessages.find(hdl.lock());
	if (connection != this->ReceivedMessages.end()) {
		if (!connection->second.empty()) {
			auto ret = connection->second.front();
			connection->second.pop();
			return ret;
		}
	}
	return std::string();
}

inline bool WebSocketProvider::Send(websocketpp::connection_hdl hdl, std::vector<uint8_t>& msg, size_t& BytesSent) {
	auto con = this->server.get_con_from_hdl(hdl);
	BytesSent = msg.size();
	auto error = con->send(msg.data(), BytesSent, websocketpp::frame::opcode::binary);
	if (error.value() != 0) {
		BytesSent = 0;
		return false;
	}
	return true;
}

inline bool WebSocketProvider::Close(websocketpp::connection_hdl hdl)
{
	this->server.close(hdl, websocketpp::close::status::going_away, "Closed By Server");
	this->ReceivedMessages.erase(hdl.lock());
	return true;
}

inline websocketpp::server<websocketpp::config::asio>* WebSocketProvider::GetServer()
{
	return &(this->server);
}

inline void WebSocketProvider::StartServer(int Port)
{
	this->server.init_asio();
	this->server.set_open_handler(std::bind(&WebSocketProvider::on_open, this, std::placeholders::_1));
	this->server.set_message_handler(std::bind(&WebSocketProvider::on_message, this, std::placeholders::_1, std::placeholders::_2));
	this->server.listen(Port);
	std::thread t_host([&]() {try { this->server.start_accept(); this->server.run(); } catch (int e) { Error("WebSocket Server Failed %d", e); }});
	t_host.detach();
	Log("WebSocket Server Started");
}
inline void WebSocketProvider::on_open(websocketpp::connection_hdl hdl) {
	Log("New Connection");
	auto con = this->server.get_con_from_hdl(hdl);
	auto uri = con->get_uri();
	if (uri && uri->get_valid()) {
		auto str = uri->str();
		Log("request: %s", str.c_str());
		auto last_slash = str.find_last_of("/");
		if (last_slash != std::string::npos) {
			std::string result = str.substr(last_slash + 1);
			Log("endpoint: %s", result.c_str());
			auto endpoint = this->Endpoints.find(result);
			if (endpoint != this->Endpoints.end()) {
				if (endpoint->second) {
					endpoint->second->set_value(hdl);
					Log("Futrue Set For Connection: %s", result.c_str());
					this->ReceivedMessages[hdl.lock()] = {};
					return;
				}
			}
		}
	}
	//Cannot Find Endpoint
	this->server.close(hdl, websocketpp::close::status::bad_gateway, "No Serice At This Endpoint");
	return;
}

inline void WebSocketProvider::on_message(websocketpp::connection_hdl hdl, websocketpp::connection<websocketpp::config::asio>::message_ptr msg)
{
	Log("New Message");
	auto connection = this->ReceivedMessages.find(hdl.lock());
	if (connection != this->ReceivedMessages.end()) {
		connection->second.push(msg->get_payload());
	}
}
