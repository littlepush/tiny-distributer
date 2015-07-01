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

#ifndef __TINY_DISTRIBUTER_SERVICES_H__
#define __TINY_DISTRIBUTER_SERVICES_H__

#include "configs.h"
#include "log.h"

// Serveice Base Class
class td_service
{
public:
	//typedef void (*td_data_redirect)(const string &);
	typedef function<void (const string &)>		td_data_redirect;
protected:
	td_config					*config_;
	sl_tcpsocket	 			server_so_;
	map<SOCKET_T, bool>			request_so_;
	map<SOCKET_T, bool> 		tunnel_so_;
	vector<td_data_redirect>	request_redirect_;
	vector<td_data_redirect>	response_redirect_;
#ifdef USE_THREAD_SERVICE
	mutable mutex				service_mutex_;
#endif
public:
	td_service();
	// Support virtual destructure
	virtual ~td_service();

	// Get the server socket
	sl_tcpsocket& server_so();

	// Get the server's name
	const string &server_name() const;

	// Check if the socket is maintained by current service
	bool is_maintaining_socket(SOCKET_T so) const;

	virtual bool start_service();
	virtual bool accept_new_incoming(SOCKET_T so) = 0;
	virtual void close_socket(SOCKET_T so) = 0;
	virtual void socket_has_data_incoming(SOCKET_T so) = 0;

	// Backdoor redirect
	void register_request_redirect(td_data_redirect redirect);
	void register_response_redirect(td_data_redirect redirect);
};

class td_service_tunnel : public td_service
{
protected:
	map<SOCKET_T, SOCKET_T>		so_map_;
	event_pool<SOCKET_T>		pool_;

#ifdef USE_THREAD_SERVICE
	vector< thread > 			workers_;
	mutex						status_lock_;
	bool						service_status_;
	// Check if service is running.
	bool _isrunning();
	void _initialize_thread_pool();
#endif
	void _did_accept_sockets(SOCKET_T src, SOCKET_T dst);
	void _read_incoming_data(SOCKET_T&& so);
public:

	// Virtual destructure
	virtual ~td_service_tunnel();

	// On socket been closed
	virtual void close_socket(SOCKET_T so);
	// Has new data coming
	virtual void socket_has_data_incoming(SOCKET_T so);
};

// Register the new service, serach progress will search all 
// registed services.
bool register_new_service(shared_ptr<td_service> service);

// Stop all running services
void stop_all_services();

// Get the service which its server socket is equal to so
shared_ptr<td_service> service_by_socket(SOCKET_T so);

// Get the service which maintain the client socket as so
shared_ptr<td_service> service_by_maintaining_socket(SOCKET_T so);

// Get the service by server name
shared_ptr<td_service> service_by_name(const string &name);

#endif // tinydst.services.h

/*
 Push Chen.
 littlepush@gmail.com
 http://pushchen.com
 http://twitter.com/littlepush
 */
