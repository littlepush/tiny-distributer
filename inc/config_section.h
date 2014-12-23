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

#ifndef __TINY_DISTRIBUTER_CONFIG_SECTION_H__
#define __TINY_DISTRIBUTER_CONFIG_SECTION_H__

#include <iostream>
#include <string>
#include <list>
#include <map>
#include <fstream>
#include <vector>

using namespace std;

// Config Section Class
class config_section
{
    map<string, string> m_config_nodes;
    map<string, config_section*> m_sub_sections;
    map<string, string>::iterator m_iterator;
    string m_name;
    int m_config_level;
    bool m_config_status;

    bool _internal_process_until_find_section_header(ifstream &in, string &head_string);
    config_section();
public:

    typedef map<string, string>::iterator _tnode;

    // Construction
    config_section(ifstream &in);
    ~config_section();

    // The section name
    string &name;

    // Check if the config has been loaded correctly.
    operator bool() const;

    // Loop to get all node
    void begin_loop();
    map<string, string>::iterator end();
    map<string, string>::iterator current_node();
    void next_node();

    // key-value operator
    string & operator[](const string &k);
    string & operator[](const char *k);
    bool contains_key(const string &k);
    void get_sub_section_names(vector<string> &names);
    config_section * sub_section(const string &name);
};

// Open a config file
config_section* open_config_file(const char* fp);
// Close a config file and release the memory
void close_config_file(config_section* cs);

// trim from start
std::string &ltrim(std::string &s);

// trim from end
std::string &rtrim(std::string &s);

// trim from both ends
std::string &trim(std::string &s);

// Split a string with the char in the carry.
void split_string( const std::string &value, 
    const std::string &carry, std::vector<std::string> &component );

#endif

/*
 Push Chen.
 littlepush@gmail.com
 http://pushchen.com
 http://twitter.com/littlepush
 */