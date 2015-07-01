/*
    tiny-distributer -- a C++ tcp distributer application for Linux/Windows/iOS
    Copyright (C) 2014  Push Chen

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

    You can connect me by email: littlepush@gmail.com, 
    or @me on twitter: @littlepush
*/

#include "services.h"

// Service
td_service::td_service() {}
td_service::~td_service() { 
	server_so_.close();
	for ( auto &_soit : request_so_ ) {
		close(_soit.first);
	}
	for ( auto &_soit : tunnel_so_ ) {
		close(_soit.first);
	}
}
sl_tcpsocket& td_service::server_so() { return server_so_; }

const string& td_service::server_name() const {
	return config_->server_name();
}
bool td_service::is_maintaining_socket(SOCKET_T so) const {
	auto _reqit = request_so_.find(so);
	if ( _reqit != request_so_.end() ) return true;
	auto _tunit = tunnel_so_.find(so);
	if ( _tunit != tunnel_so_.end() ) return true;
	return false;
}
bool td_service::start_service() {
	for ( int i = 0; i < 30; ++i ) {
		if (server_so_.listen(config_->server_port(), config_->local_ip())) {
			sl_poller::server().bind_tcp_server(server_so_.m_socket);
			return true;
		}
		sleep(1);
	}
	return false;
}
bool td_service::accept_new_incoming(SOCKET_T so) {
	uint32_t _ipaddr, _port;
	network_peer_info_from_socket(so, _ipaddr, _port);
	return config_->is_ip_in_range(_ipaddr);
}

void td_service::register_request_redirect(td_service::td_data_redirect redirect) {
	request_redirect_.push_back(redirect);
}
void td_service::register_response_redirect(td_service::td_data_redirect redirect) {
	response_redirect_.push_back(redirect);
}

// Tunnel Service
#ifdef USE_THREAD_SERVICE
bool td_service_tunnel::_isrunning() {
	unique_lock<mutex> _l(status_lock_);
	return service_status_;
}
void td_service_tunnel::_initialize_thread_pool() {
	service_status_ = true;
	for ( uint32_t i = 0; i < config_->thread_pool_size(); ++i ) {
		workers_.emplace_back(
			[this](){
				while ( this->_isrunning() ) {
					SOCKET_T _so;
					bool _status = pool_.wait_for(chrono::milliseconds(100), [&](SOCKET_T && rv) {
						_so = rv;
					});
					if ( _status == false ) continue;
					this->_read_incoming_data(move(_so));
				}
			}
		);
	}
}
#endif
td_service_tunnel::~td_service_tunnel() {
#ifdef USE_THREAD_SERVICE
	{
		unique_lock<mutex> _l(status_lock_);
		service_status_ = false;
	}
	for ( auto & _wt : workers_ ) {
		_wt.join();
	}
#endif
}

void td_service_tunnel::_did_accept_sockets(SOCKET_T src, SOCKET_T dst) {
	request_so_[src] = true;
	tunnel_so_[dst] = true;
	so_map_[src] = dst;
	so_map_[dst] = src;
#ifdef USE_THREAD_SERVICE
	sl_poller::server().monitor_socket(src, true);
	sl_poller::server().monitor_socket(dst, true);
#else
	sl_poller::server().monitor_socket(src, false);
	sl_poller::server().monitor_socket(dst, false);
#endif
}

void td_service_tunnel::close_socket(SOCKET_T so) {
	auto _peer = so_map_.find(so);
	if ( _peer == so_map_.end() ) return;
	// Close and clear
	close(so);
	close(_peer->second);
	request_so_.erase(so);
	tunnel_so_.erase(so);
	request_so_.erase(_peer->second);
	so_map_.erase(so);
	so_map_.erase(_peer->second);
}

void td_service_tunnel::socket_has_data_incoming(SOCKET_T so) {
	// Enqueue the event
#ifdef USE_THREAD_SERVICE
	pool_.notify_one(move(so));
#else
	this->_read_incoming_data(move(so));
#endif
}

void td_service_tunnel::_read_incoming_data(SOCKET_T&& so) {
	sl_tcpsocket _wrapso(so);
	auto _peer = so_map_.find(so);
	sl_tcpsocket _wrapdso(_peer->second);
	string _buf;
	SO_READ_STATUE _st;

#ifdef USE_THREAD_SERVICE
	while ( this->_isrunning() ) {
#else
	while ( true ) {
#endif
		_st = _wrapso.read_data(_buf, 1000);
		// If no data
		if ( (_st & SO_READ_DONE) == 0 ) break;
		_wrapdso.write_data(_buf);

		if ( request_so_.find(so) != request_so_.end() ) {
			// This is a request socket
			// broadcast to all backdoor services
			for ( auto _rd : request_redirect_ ) {
				_rd(_buf);
			}
		}
		if ( tunnel_so_.find(so) != tunnel_so_.end() ) {
			// This is a tunnel socket, which means is a response data.
			// broadcast to all backdoor services
			for ( auto _rd : response_redirect_ ) {
				_rd(_buf);
			}
		}

		// Which means unfinished
		if ( _st & SO_READ_TIMEOUT ) continue;
		break;
	}
	if ( _st & SO_READ_CLOSE ) {
		this->close_socket(so);
	}
#if USE_THREAD_SERVICE
	else {
		sl_poller::server().monitor_socket(so, true, true);
	}
#endif
}

vector<shared_ptr<td_service>> &g_service_list() {
	static vector<shared_ptr<td_service>> _l;
	return _l;
}
bool register_new_service(shared_ptr<td_service> service) {
	if ( service->start_service() == false ) return false;
	g_service_list().push_back(service);
	return true;
}
void stop_all_services( ) {
	g_service_list().clear();
}
shared_ptr<td_service> service_by_socket(SOCKET_T so) {
	for ( auto &_ptr : g_service_list() ) {
		if ( _ptr->server_so().m_socket == so ) return _ptr;
	}
	return shared_ptr<td_service>(nullptr);
}
shared_ptr<td_service> service_by_maintaining_socket(SOCKET_T so) {
	for ( auto &_ptr : g_service_list() ) {
		if ( _ptr->is_maintaining_socket(so) ) return _ptr;
	}
	return shared_ptr<td_service>(nullptr);
}
shared_ptr<td_service> service_by_name(const string &name) {
	for ( auto &_ptr : g_service_list() ) {
		if ( _ptr->server_name() == name ) return _ptr;
	}
	return shared_ptr<td_service>(nullptr);
}


/*
 Push Chen.
 littlepush@gmail.com
 http://pushchen.com
 http://twitter.com/littlepush
 */
