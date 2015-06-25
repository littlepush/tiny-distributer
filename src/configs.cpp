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

#include "configs.h"

// trim from start
static inline std::string &ltrim(std::string &s) {
    s.erase(
        s.begin(), 
        std::find_if(
            s.begin(), 
            s.end(), 
            std::not1(std::ptr_fun<int, int>(std::isspace))
            )
        );
    return s;
}

// trim from end
static inline std::string &rtrim(std::string &s) {
    s.erase(
        std::find_if(
            s.rbegin(), 
            s.rend(), 
            std::not1(std::ptr_fun<int, int>(std::isspace))
            ).base(), 
        s.end()
        );
    return s;
}

// trim from both ends
static inline std::string &trim(std::string &s) {
    return ltrim(rtrim(s));
}

// Split a string with the char in the carry.
static inline void split_string( const std::string &value, 
    const std::string &carry, std::vector<std::string> &component )
{
    if ( value.size() == 0 ) return;
    std::string::size_type _pos = 0;
    do {
        std::string::size_type _lastPos = std::string::npos;
        for ( std::string::size_type i = 0; i < carry.size(); ++i ) {
            std::string::size_type _nextCarry = value.find( carry[i], _pos );
            _lastPos = (_nextCarry < _lastPos) ? _nextCarry : _lastPos;
        }
        if ( _lastPos == std::string::npos ) _lastPos = value.size();
        if ( _lastPos > _pos ) {
            std::string _com = value.substr( _pos, _lastPos - _pos );
            component.push_back(_com);
        }
        _pos = _lastPos + 1;
    } while( _pos < value.size() );
}

static uint32_t g_string_to_ipint(const string &ipstring) {
	vector<string> _components;
	string _clean_ipstring(ipstring);
	trim(_clean_ipstring);
	// Split the ip address
	split_string(_clean_ipstring, ".", _components);
	if ( _components.size() != 4 ) return 0;

	uint32_t _ipaddr = 0;
	for ( int i = 0; i < 4; ++i ) {
		int _i = stoi(_components[i], nullptr, 10);
		if ( _i < 0 || _i > 255 ) return 0;
		_ipaddr |= (_i << ((3 - i) * 8));
	}
	return _ipaddr;
}

static td_iprange g_string_to_range(const string &rangestring) {
	ostringstream _oss;
	string _clean_rangestring(rangestring);
	trim(_clean_rangestring);

	vector<string> _com;
	split_string(_clean_rangestring, "/", _com);
	if ( _com.size() == 1 ) _com.push_back("32");

	// IP part
	string _ipstr = _com[0];
	vector<string> _ipcom;
	split_string(_ipstr, ".", _ipcom);
	for ( int i = 0; i < (_ipcom.size() - 4); ++i ) {
		_ipcom.push_back("0");
	}
	uint32_t _ipaddr = 0;
	for ( int i = 0; i < 4; ++i ) {
		int _i = stoi(_ipcom[i], nullptr, 10);
		if ( _i < 0 || _i > 255 ) {
			_oss << "invalidate ip-range string: " << rangestring;
			throw(runtime_error(_oss.str()));
		}
		_ipaddr |= (_i << ((3 - i) * 8));
	}

	// Mask part
	string _maskstr = _com[1];
	uint32_t _mask = 0xFFFFFFFF;
	if ( _maskstr.size() > 2 ) {
		_mask = g_string_to_ipint(_maskstr);
		if ( _mask == 0 ) {
			_oss << "invalidate ip-range string: " << rangestring << ", mask error";
			throw(runtime_error(_oss.str()));
		}
	} else {
		int _m = stoi(_maskstr, nullptr, 10);
		if ( _m <= 0 || _m > 32 ) {
			_oss << "invalidate ip-range string: " << rangestring << ", mask not in range";
			throw(runtime_error(_oss.str()));
		}
		_mask = 1 << _m;
	}

	uint32_t _low = _ipaddr & _mask;
	uint32_t _high = _low | (~_mask);
	return make_pair(_low, _high);
}

static td_peerinfo g_object_to_peerinfo(const Json::Value &object) {
	if ( object.isMember("addr") == false ) {
		throw(runtime_error("missing \"addr\" in peer info"));
	} 
	if ( object.isMember("port") == false ) {
		throw(runtime_error("missing \"port\" in peer info"));
	}
	Json::Value _addr = object["addr"];
	Json::Value _port = object["port"];
	uint32_t _ipaddr = g_string_to_ipint(_addr.asString());
	if ( _ipaddr == 0 ) {
		throw(runtime_error("invalidate ip address in peer info"));
	}
	uint32_t _p = _port.asUInt();
	if ( _p == 0 || _p > 65535 ) {
		throw(runtime_error("invalidate port in peer info"));
	}
	uint16_t _uport = (uint16_t)_p;
	return make_pair(_ipaddr, _uport);
}

static void g_object_to_socks5list(const Json::Value &socks5obj, vector<td_peerinfo> &proxylist) {
	if ( socks5obj.isObject() ) {
		proxylist.push_back(g_object_to_peerinfo(socks5obj));
	} else if ( socks5obj.isArray() ) {
		for ( Json::ArrayIndex i = 0; i < socks5obj.size(); ++i ) {
			proxylist.push_back(g_object_to_peerinfo(socks5obj[i]));
		}
	} else {
		throw(runtime_error("invalidate socks5 setting"));
	}
}

static td_dst g_object_to_dst(const Json::Value &object) {
	td_dst _dst;
	td_peerinfo _pi = g_object_to_peerinfo(object);
	_dst.ipaddr = _pi.first;
	_dst.port = _pi.second;
	_dst.accept_reply = (object.isMember("reply") ? object["reply"].asBool() : true);
	if ( object.isMember("socks5") ) {
		g_object_to_socks5list(object["socks5"], _dst.socks5);
	}
	return _dst;
}

td_dst::td_dst(): accept_reply(true) {

}

// Basic Config 
td_config::td_server td_config::server_type_from_string(const string &typestring) {
	static map<string, td_server> _map;
	if ( _map.size() == 0 ) {
		_map["TCPRELAY"] = td_server_tcprelay;
		_map["REDIRECT"] = td_server_redirect;
		_map["SOCKS5"] = td_server_socks5;
		_map["BACKDOOR"] = td_server_backdoor;
	}
	auto _itype = _map.find(typestring);
	if ( _itype == _map.end() ) return td_server_unknow;
	return _itype->second;
}

td_config::td_config(const string &name, const Json::Value &config_node)
: server_name_(name) {
	ostringstream _oss;
	if ( config_node.isMember("server") == false ) {
		_oss << "missing \"server\" in section " << server_name_;
		throw(runtime_error(_oss.str()));
	}
	string _server_type = config_node["server"].asString();
	type_ = server_type_from_string(_server_type);
	if ( type_ == td_server_unknow ) {
		_oss << "unknow server type (" << _server_type << ") for section " << server_name_;
		throw(runtime_error(_oss.str()));
	}
	if ( config_node.isMember("port") == false ) {
		_oss << "missing \"port\" in section " << server_name_;
		throw(runtime_error(_oss.str()));
	}
	port_ = (uint16_t)config_node["port"].asUInt();
	// In default, listen on all interface
	local_ip_ = INADDR_ANY;
	// Bind to specified interface
	if ( config_node.isMember("bind") == true ) {
		string _bindip = config_node["bind"].asString();
		local_ip_ = g_string_to_ipint(_bindip);
		if ( local_ip_ == 0 ) {
			_oss << "invalidate bind local ip: " << _bindip;
			throw(runtime_error(_oss.str()));
		}
	}
	// Source
	if ( config_node.isMember("src") == true ) {
		Json::Value _src_node = config_node["src"];
		if ( _src_node.isString() ) {
			source_range_.push_back(g_string_to_range(_src_node.asString()));
		} else if ( _src_node.isArray() ) {
			for ( Json::ArrayIndex i = 0; i < _src_node.size(); ++i ) {
				source_range_.push_back(g_string_to_range(_src_node[i].asString()));
			}
		}
	}
}
td_config::~td_config() {
	// nothing
}

const string &td_config::server_name() const { return server_name_; }
uint32_t td_config::local_ip() const { return local_ip_; }
uint16_t td_config::server_port() const { return port_; }
bool td_config::is_ip_in_range(uint32_t ip) const {
	if ( source_range_.size() == 0 ) return true;
	for ( auto &_range : source_range_ ) {
		if ( _range.first <= ip && _range.second >= ip ) return true;
	}
	return false;
}

// TCP Relay Server Config
td_config_tcprelay::td_config_tcprelay(const string &name, const Json::Value &config_node)
	:td_config(name, config_node) 
{
	// No socks5 config
	if ( config_node.isMember("socks5") == false ) return;
	
	Json::Value _socks5_config = config_node["socks5"];
	g_object_to_socks5list(_socks5_config, socks5_proxy_);
}

const vector<td_peerinfo>& td_config_tcprelay::proxy_list() const { return socks5_proxy_; }

// Redirect server config
td_config_redirect::td_config_redirect(const string &name, const Json::Value &config_node) 
	:td_config(name, config_node)
{
	if ( config_node.isMember("dst") == false ) {
		throw(runtime_error("missing \"dst\" in redirect server"));
	}
	const Json::Value &_dst_config = config_node["dst"];
	if ( _dst_config.isObject() ) {
		dst_.push_back(g_object_to_dst(_dst_config));
	} else if ( _dst_config.isArray() ) {
		for ( Json::ArrayIndex i = 0; i < _dst_config.size(); ++i ) {
			dst_.push_back(g_object_to_dst(_dst_config[i]));
		}
	} else {
		throw(runtime_error("invalidate dst config"));
	}
}

const vector<td_dst>& td_config_redirect::destination_list() const { return dst_; }

// Socks5 Server Config
td_config_socks5::td_config_socks5(const string &name, const Json::Value &config_node)
	:td_config(name, config_node)
{
	// No socks5 config
	if ( config_node.isMember("socks5") == false ) return;

	Json::Value _socks5_config = config_node["socks5"];
	g_object_to_socks5list(_socks5_config, socks5_proxy_);

	noauth_ = (config_node.isMember("noauth") ? config_node["noauth"].asBool() : true);
	if ( config_node.isMember("validation") ) {
		Json::Value _validation = config_node["validation"];
		for ( Json::ValueIterator _it = _validation.begin(); _it != _validation.end(); ++_it ) {
			usrpwd_map_[_it.key().asString()] = _validation[_it.key().asString()].asString();
		}
	}
}

const vector<td_peerinfo>& td_config_socks5::proxy_list() const { return socks5_proxy_; }
bool td_config_socks5::is_support_noauth() const { return noauth_; }
bool td_config_socks5::is_validate_user(const string &username, const string &password) const {
	auto _it = usrpwd_map_.find(username);
	if ( _it == end(usrpwd_map_) ) return false;
	if ( _it->second != password ) return false;
	return true;
}

// Backdoor server Config
td_config_backdoor::td_config_backdoor(const string &name, const Json::Value &config_node)
	:td_config(name, config_node), accept_request_(true), accept_response_(true)
{
	if ( config_node.isMember("listen") == false ) {
		throw(runtime_error("the backdoor server not related to any server"));
	}
	related_name_ = config_node["listen"].asString();
	if ( config_node.isMember("request") ) {
		accept_request_ = config_node["request"].asBool();
	}
	if ( config_node.isMember("response") ) {
		accept_response_ = config_node["response"].asBool();
	}
}

const string& td_config_backdoor::related_server_name() const { return related_name_; }
bool td_config_backdoor::is_redirect_request() const { return accept_request_; }
bool td_config_backdoor::is_redirect_response() const { return accept_response_; }

// Service
sl_tcpsocket& td_service::server_so() { return server_so_; }

bool td_service::is_maintaining_socket(SOCKET_T so) const {
	auto _reqit = request_so_.find(so);
	if ( _reqit != request_so_.end() ) return true;
	auto _tunit = tunnel_so_.find(so);
	if ( _tunit != tunnel_so_.end() ) return true;
	return false;
}
void td_service::registe_request_redirect(td_service::td_data_redirect redirect) {
	request_redirect_.push_back(redirect);
}
void td_service::registe_response_redirect(td_service::td_data_redirect redirect) {
	response_redirect_.push_back(redirect);
}

vector<shared_ptr<td_service>> &g_service_list() {
	static vector<shared_ptr<td_service>> _l;
	return _l;
}
bool registe_new_service(shared_ptr<td_service> service) {
	if ( service->start_service() == false ) return false;
	g_service_list().push_back(service);
	return true;
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

// tinydst.configs.cpp

/*
 Push Chen.
 littlepush@gmail.com
 http://pushchen.com
 http://twitter.com/littlepush
 */
