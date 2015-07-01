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
#ifdef USE_THREAD_SERVICE
	unique_lock<mutex> _l(service_mutex_);
#endif
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
		td_log(log_error, "server(%s) failed to listen on port %u, retrying...", 
				this->server_name().c_str(), config_->server_port());
		sleep(1);
	}
	td_log(log_error, "server(%s) failed to listen on port %u, failed to start.", 
			this->server_name().c_str(), config_->server_port());
	return false;
}
bool td_service::accept_new_incoming(SOCKET_T so) {
	uint32_t _ipaddr, _port;
	network_peer_info_from_socket(so, _ipaddr, _port);
	bool _ret = config_->is_ip_in_range(_ipaddr);
	td_log(log_notice, "server(%s) check incoming[%d.%d.%d.%d:%u]: %s",
			this->server_name().c_str(),
			(_ipaddr >> (0 * 8)) & 0x00FF,
			(_ipaddr >> (1 * 8)) & 0x00FF,
			(_ipaddr >> (2 * 8)) & 0x00FF,
			(_ipaddr >> (3 * 8)) & 0x00FF,
			_port,
			(_ret ? "accept" : "deny"));
	return _ret;
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
				td_log(log_info, "server(%s) start thread %u", 
						this->server_name().c_str(),
						this_thread::get_id());
				while ( this->_isrunning() ) {
					SOCKET_T _so;
					bool _status = pool_.wait_for(chrono::milliseconds(20), [&](SOCKET_T && rv) {
						td_log(log_debug, "server(%s) worker[%u] fetch so %d", 
							this->server_name().c_str(), this_thread::get_id(), rv);
						_so = rv;
					});
					if ( _status == false ) continue;
					this->_read_incoming_data(move(_so));
				}
				td_log(log_info, "server(%s) stop thread %u", 
						this->server_name().c_str(), 
						this_thread::get_id());
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
	td_log(log_debug, "server(%s) did accept incoming so(%d) and relay so(%d)", 
			this->server_name().c_str(), src, dst);
#ifdef USE_THREAD_SERVICE
	lock_guard<mutex> _l(service_mutex_);
#endif
	request_so_[src] = true;
	tunnel_so_[dst] = true;
	so_map_[src] = dst;
	so_map_[dst] = src;
#ifdef USE_THREAD_SERVICE
	sl_poller::server().monitor_socket(src, true, false);
	sl_poller::server().monitor_socket(dst, true, false);
#else
	sl_poller::server().monitor_socket(src, false, false);
	sl_poller::server().monitor_socket(dst, false, false);
#endif
}

void td_service_tunnel::close_socket(SOCKET_T so) {
#ifdef USE_THREAD_SERVICE
	lock_guard<mutex> _l(service_mutex_);
#endif
	auto _peer = so_map_.find(so);
	if ( _peer == so_map_.end() ) return;
	// Close and clear
	td_log(log_debug, "server(%s) has close socket %d relay with %d", 
			this->server_name().c_str(), so, _peer->second);
	request_so_.erase(so);
	tunnel_so_.erase(so);
	request_so_.erase(_peer->second);
	so_map_.erase(so);
	so_map_.erase(_peer->second);
	close(so);
	close(_peer->second);
}

void td_service_tunnel::socket_has_data_incoming(SOCKET_T so) {
	// Enqueue the event
	td_log(log_debug, "server(%s) processing data reading for so: %d", 
			this->server_name().c_str(), so);
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

	td_log(log_debug, "server(%s) begin to read so: %d", 
			this->server_name().c_str(), so);

#ifdef USE_THREAD_SERVICE
	_st = _wrapso.recv(_buf, 1024);
	td_log(log_debug, "server(%s) did recv from so: %d, st: 0x%02x, buf size: %u", 
			this->server_name().c_str(), so, _st, _buf.size());
	if ( _st & SO_READ_DONE ) {
#else
	while ( true ) {
		_st = _wrapso.read_data(_buf, 1000);
		td_log(log_debug, "server(%s) did read from so: %d, st: 0x%02x, buf size: %u", 
				this->server_name().c_str(), so, _st, _buf.size());
		// If no data
		if ( (_st & SO_READ_DONE) == 0 ) break;
#endif
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

#ifdef USE_THREAD_SERVICE
		unique_lock<mutex> _l(service_mutex_);
		if ( so_map_.find(so) == so_map_.end() ) {
			_l.unlock();
			td_log(log_debug, "server(%s) has already close so(%d)",
					this->server_name().c_str(), so);
			_wrapso.close();
			_st = SO_READ_WAITING;
		} else {
			td_log(log_debug, "server(%s) put so(%d) back to poller", 
					this->server_name().c_str(), so);
			sl_poller::server().monitor_socket(so, true, true);
		}
#else
		// Which means unfinished
		if ( _st & SO_READ_TIMEOUT ) {
			td_log(log_debug, "server(%s) read time on so(%s), try more",
					this->server_name().c_str(), so);
			continue;
		}
		td_log(log_debug, "server(%s) read done on so(%s)", 
				this->server_name().c_str(), so);
		break;
#endif
	}
	if ( _st & SO_READ_CLOSE ) {
		this->close_socket(so);
	}
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
