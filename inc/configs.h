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

#pragma once

#ifndef __TINY_DISTRIBUTER_CONFIG_H__
#define __TINY_DISTRIBUTER_CONFIG_H__

#include <iostream>
#include <string>
#include <map>
#include <list>
#include <vector>
#include <fstream>
#include <memory>
#include <algorithm>

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>

#include "json/json.h"
#include "json/json-forwards.h"
#include "socklite/socketlite.h"

#include "thread.h"

using namespace std;

typedef pair<string, uint16_t>		td_peerinfo;
typedef pair<uint32_t, uint32_t>	td_iprange;

typedef struct td_dst {
	string				ipaddr;
	uint16_t			port;
	bool				accept_reply;
	vector<td_peerinfo>	socks5;

	// in default appcpt_reply = true
	td_dst();
} td_dst;

class td_config
{
public:
	enum { 
		td_server_unknow,
		td_server_tcprelay,
		td_server_redirect,
		td_server_socks5,
		td_server_backdoor
	};
	typedef int td_server;
protected:
	td_server					type_;
	string						server_name_;
	uint32_t					local_ip_;
	uint16_t					port_;
	vector<td_iprange>			source_range_;
	uint32_t					thread_pool_size_;
	uint32_t					buffer_size_;
public:
	static td_server server_type_from_string(const string &typestring);
public:
	td_config(const string &name, const Json::Value &config_node);
	virtual ~td_config();

	const string &server_name() const;
	uint32_t local_ip() const;
	uint16_t server_port() const;
	uint32_t thread_pool_size() const;
	bool is_ip_in_range(uint32_t ip) const;
	uint32_t socket_buffer_size() const;
};

class td_config_tcprelay : public td_config
{
protected:
	vector<td_peerinfo>			socks5_proxy_;

#ifdef USE_SOCKS_WHITELIST
	vector<td_iprange>			socks5_whitelist_;
#endif
public:
	td_config_tcprelay(const string &name, const Json::Value &config_node);

	const vector<td_peerinfo>& proxy_list() const;
#ifdef USE_SOCKS_WHITELIST
	bool is_ip_in_whitelist(uint32_t ipaddr) const;
#endif
};

class td_config_redirect : public td_config
{
protected:
	vector<td_dst>				dst_;
public:
	td_config_redirect(const string &name, const Json::Value &config_node);

	const vector<td_dst>& destination_list() const;
};

class td_config_socks5 : public td_config
{
protected:
	vector<td_peerinfo>			socks5_proxy_;
	bool						noauth_;
	map<string, string>			usrpwd_map_;
public:
	td_config_socks5(const string &name, const Json::Value &config_node);

	const vector<td_peerinfo>& proxy_list() const;
	bool is_support_noauth() const;
	bool is_validate_user(const string &username, const string &password) const;
};

class td_config_backdoor : public td_config
{
protected:
	string						related_name_;
	bool						accept_request_;
	bool						accept_response_;
public:
	td_config_backdoor(const string &name, const Json::Value &config_node);

	const string &related_server_name() const;
	bool is_redirect_request() const;
	bool is_redirect_response() const;
};

#endif // tinydst.configs.h

/*
 Push Chen.
 littlepush@gmail.com
 http://pushchen.com
 http://twitter.com/littlepush
 */
