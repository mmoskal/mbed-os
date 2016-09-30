/* Socket
 * Copyright (c) 2015 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "network-socket/NetworkInterface.h"
#include "network-socket/NetworkStack.h"
#include <string.h>


// Default network-interface state
const char *NetworkInterface::get_mac_address()
{
    return 0;
}

const char *NetworkInterface::get_ip_address()
{
    return 0;
}

const char *NetworkInterface::get_netmask()
{
    return 0;
}

const char *NetworkInterface::get_gateway()
{
    return 0;
}

int NetworkInterface::set_network(const char *ip_address, const char *netmask, const char *gateway)
{
    return NSAPI_ERROR_UNSUPPORTED;
}

int NetworkInterface::set_dhcp(bool dhcp)
{
    if (!dhcp) {
        return NSAPI_ERROR_UNSUPPORTED;
    } else {
        return 0;
    }
}

// DNS operations go through the underlying stack by default
int NetworkInterface::gethostbyname(const char *name, SocketAddress *address)
{
    return get_stack()->gethostbyname(name, address);
}

int NetworkInterface::gethostbyname(const char *name, SocketAddress *address, nsapi_version_t version)
{
    return get_stack()->gethostbyname(name, address, version);
}

int NetworkInterface::add_dns_server(const SocketAddress &address)
{
    return get_stack()->add_dns_server(address);
}
