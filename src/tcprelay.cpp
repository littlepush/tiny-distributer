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

#define IP_WHITELIST_TEST(domain)				\
	cout << domain << ": " << (_cfg->is_ip_in_whitelist(htonl(network_domain_to_inaddr(domain))) ? "yes" : "no") << endl


td_service_tcprelay::td_service_tcprelay(const string &name, const Json::Value &config_node)
{
	config_ = new td_config_tcprelay(name, config_node);

	td_config_tcprelay *_cfg = static_cast<td_config_tcprelay *>(config_);
	IP_WHITELIST_TEST("www.baidu.com");
	IP_WHITELIST_TEST("www.taobao.com");
	IP_WHITELIST_TEST("www.jd.com");
	IP_WHITELIST_TEST("www.google.com");
	IP_WHITELIST_TEST("twitter.com");
	IP_WHITELIST_TEST("www.facebook.com");
	IP_WHITELIST_TEST("pushchen.com");
	IP_WHITELIST_TEST("www.tmall.com");
	IP_WHITELIST_TEST("github.com");

	exit(0);

#ifdef USE_THREAD_SERVICE
	this->_initialize_thread_pool();
#endif
}
td_service_tcprelay::~td_service_tcprelay() {
	delete config_;
}
bool td_service_tcprelay::accept_new_incoming(SOCKET_T so) {
	sl_tcpsocket _wrap_src(so);
	if ( !td_service::accept_new_incoming(so) ) {
		_wrap_src.close();
		return false;
	}

	// Get original destination
	string _org_addr;
	uint32_t _org_port;
	if ( !_wrap_src.get_original_dest(_org_addr, _org_port) ) {
		// The socket is not validate
		_wrap_src.close();
		return false;
	}

	sl_tcpsocket _wrap_dst(true);
	td_config_tcprelay *_cfg = static_cast<td_config_tcprelay *>(config_);
	bool _ret = false;

#ifdef USE_SOCKS_WHITELIST
	struct sockaddr_in _saddr;
	inet_pton(AF_INET, _org_addr.c_str(), &(_saddr.sin_addr));
	uint32_t _ipaddr = (uint32_t)(_saddr.sin_addr.s_addr);
	_ipaddr = htonl(_ipaddr);
	if ( _cfg->is_ip_in_whitelist(htonl(_ipaddr)) ) {
		td_log(log_debug, "%s: %s is in white list",
			   this->server_name().c_str(),	_org_addr.c_str());
		_ret = _wrap_dst.connect(_org_addr, _org_port);
	} else {
#endif

#ifdef AUTO_TCPRELAY_SOCKS5
#ifndef TCPRELAY_DIRECT_TIMEOUT
#define TCPRELAY_DIRECT_TIMEOUT	300
#endif
	if ( !_wrap_dst.connect(_org_addr, _org_port, TCPRELAY_DIRECT_TIMEOUT) ) {
		td_log(log_debug, "server(%s) cannot direct connect to %s:%u, will use socks5 proxy",
				this->server_name().c_str(), _org_addr.c_str(), _org_port);
		for ( auto &_socks5 : _cfg->proxy_list() ) {
			if ( _wrap_dst.setup_proxy(_socks5.first, _socks5.second) ) break;
			td_log(log_debug, "%s: failed to connect to proxy %s:%u", 
					this->server_name().c_str(), _socks5.first.c_str(), _socks5.second);
		}
		_ret = _wrap_dst.connect(_org_addr, _org_port);
	} else {
		td_log(log_debug, "server(%s) did connect to %s:%u by direct connection",
				this->server_name().c_str(), _org_addr.c_str(), _org_port);
	}
#else
	for ( auto &_socks5 : _cfg->proxy_list() ) {
		if ( _wrap_dst.setup_proxy(_socks5.first, _socks5.second) ) break;
	}
	_ret = _wrap_dst.connect(_org_addr, _org_port);
#endif

#ifdef USE_SOCKS_WHITELIST
	}
#endif

	if ( _ret ) {
		// Add to monitor
		this->_did_accept_sockets(_wrap_src.m_socket, _wrap_dst.m_socket);
	} else {
		_wrap_src.close();
		_wrap_dst.close();
	}

	// Return the status
	return _ret;
}

// tinydst.tcprelay.cpp

/*
 Push Chen.
 littlepush@gmail.com
 http://pushchen.com
 http://twitter.com/littlepush
 */
