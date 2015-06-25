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

#include "tcprelay.h"

const string td_service_tcprelay::service_name() const {
	return config_.server_name();
}
td_service_tcprelay::td_service_tcprelay(const string &name, const Json::Value &config_node)
	: config_(name, config_node)
{

}

bool td_service_tcprelay::start_service() {
	for ( int i = 0; i < 30; ++i ) {
		if (server_so_.listen(config_.server_port(), config_.local_ip())) return true;
		sleep(1);
	}
	return false;
}

void td_service_tcprelay::accept_new_incoming(SOCKET_T so) {
	sl_tcpsocket _wrap_src(so);
	// Check ip-range
	uint32_t _ipaddr, _port;
	network_peer_info_from_socket(so, _ipaddr, _port);
	if ( config_.is_ip_in_range(_ipaddr) == false ) {
		_wrap_src.close();
		return;
	}

	// Get original destination
	string _org_addr;
	uint32_t _org_port;
	if ( !_wrap_src.get_original_dest(_org_addr, _org_port) ) {
		// The socket is not validate
		_wrap_src.close();
		return;
	}

	sl_tcpsocket _wrap_dst(true);
	for ( auto &_socks5 : config_.proxy_list() ) {
		string _ipaddr;
		network_int_to_ipaddress(_socks5.first, _ipaddr);
		if ( _wrap_dst.setup_proxy(_ipaddr, _socks5.second) ) break;
	}
	if ( !_wrap_dst.connect(_org_addr, _org_port) ) {
		_wrap_dst.close();
		_wrap_src.close();
		return;
	}
	request_so_[_wrap_src.m_socket] = true;
	tunnel_so_[_wrap_dst.m_socket] = true;
	so_map_[_wrap_src.m_socket] = _wrap_dst.m_socket;
	so_map_[_wrap_dst.m_socket] = _wrap_src.m_socket;
	sl_poller::server().monitor_socket(_wrap_src.m_socket);
	sl_poller::server().monitor_socket(_wrap_dst.m_socket);
}

void td_service_tcprelay::close_socket(SOCKET_T so) {
	auto _peer = so_map_.find(so);
	if ( _peer == so_map_.end() ) return;
	// Close and clear
	close(so);
	close(_peer->second);
	request_so_.erase(so);
	tunnel_so_.erase(so);
	request_so_.erase(_peer->second);
	tunnel_so_.erase(_peer->second);
	so_map_.erase(so);
	so_map_.erase(_peer->second);
}

void td_service_tcprelay::socket_has_data_incoming(SOCKET_T so) {
	sl_tcpsocket _wrapso(so);
	auto _peer = so_map_.find(so);
	sl_tcpsocket _wrapdso(_peer->second);
	string _buf;
	if ( _wrapso.read_data(_buf) ) {
		_wrapdso.write_data(_buf);

		if ( request_so_.find(so) != request_so_.end() ) {
			// This is a request socket
			// broadcast to all backdoor services
		}
		if ( tunnel_so_.find(so) != tunnel_so_.end() ) {
			// This is a tunnel socket, which means is a response data.
			// broadcast to all backdoor services
		}
	}
}

// tinydst.tcprelay.cpp

/*
 Push Chen.
 littlepush@gmail.com
 http://pushchen.com
 http://twitter.com/littlepush
 */
