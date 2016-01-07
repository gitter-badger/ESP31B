/*
  WiFiUdp.cpp - UDP client/server for ESP31B, mostly compatible
                   with Arduino WiFi shield library

  Copyright (c) 2015 Ivan Grokhotkov. All rights reserved.
  This file is part of the ESP31B core for Arduino environment.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#define LWIP_INTERNAL
#include <functional>

extern "C"
{
    #include "include/wl_definitions.h"
    #include "esp_common.h"
}

#include "ESP31BWiFi.h"
#include "WiFiUdp.h"
#include "lwip/opt.h"
#include "lwip/udp.h"
#include "lwip/inet.h"
#include "lwip/igmp.h"
#include "lwip/mem.h"
#include "include/UdpContext.h"


template<>
WiFiUDP* SList<WiFiUDP>::_s_first = 0;

/* Constructor */
WiFiUDP::WiFiUDP() : _ctx(0)
{
    WiFiUDP::_add(this);
}

WiFiUDP::WiFiUDP(const WiFiUDP& other)
{
    _ctx = other._ctx;
    if (_ctx)
        _ctx->ref();
    WiFiUDP::_add(this);
}

WiFiUDP& WiFiUDP::operator=(const WiFiUDP& rhs)
{
    _ctx = rhs._ctx;
    if (_ctx)
        _ctx->ref();
    return *this;
}

WiFiUDP::~WiFiUDP()
{
    WiFiUDP::_remove(this);
    if (_ctx)
        _ctx->unref();
}

/* Start WiFiUDP socket, listening at local port */
uint8_t WiFiUDP::begin(uint16_t port)
{
    if (_ctx) {
        _ctx->unref();
        _ctx = 0;
    }

    _ctx = new UdpContext;
    _ctx->ref();
    ip_addr_t addr;
    addr.addr = INADDR_ANY;
    return (_ctx->listen(addr, port)) ? 1 : 0;
}

uint8_t WiFiUDP::beginMulticast(IPAddress interfaceAddr, IPAddress multicast, uint16_t port)
{
    if (_ctx) {
        _ctx->unref();
        _ctx = 0;
    }

    //ip_addr_t ifaddr;
    //ifaddr.addr = (uint32_t) interfaceAddr;
    //ip_addr_t multicast_addr;
    //multicast_addr.addr = (uint32_t) multicast;
    //if (igmp_joingroup(&ifaddr, &multicast_addr)!= ERR_OK) return 0;

    _ctx = new UdpContext;
    _ctx->ref();
    if (!_ctx->listen(*IP_ADDR_ANY, port)) {
        return 0;
    }

    return 1;
}

/* return number of bytes available in the current packet,
   will return zero if parsePacket hasn't been called yet */
int WiFiUDP::available() {
    int result = 0;

    if (_ctx) {
        result = static_cast<int>(_ctx->getSize());
    }
    return result;
}

/* Release any resources being used by this WiFiUDP instance */
void WiFiUDP::stop()
{
    if (_ctx) {
        _ctx->disconnect();
        _ctx->unref();
    }
    _ctx = 0;
}

int WiFiUDP::beginPacket(const char *host, uint16_t port)
{
    IPAddress remote_addr;
    if (WiFi.hostByName(host, remote_addr))
    {
        return beginPacket(remote_addr, port);
    }
    return 0;
}

int WiFiUDP::beginPacket(IPAddress ip, uint16_t port)
{
    ip_addr_t addr;
    addr.addr = ip;

    if (!_ctx) {
        _ctx = new UdpContext;
        _ctx->ref();
    }
    return (_ctx->connect(addr, port)) ? 1 : 0;
}

int WiFiUDP::beginPacketMulticast(IPAddress multicastAddress, uint16_t port,
    IPAddress interfaceAddress, int ttl)
{
    ip_addr_t mcastAddr;
    mcastAddr.addr = multicastAddress;
    ip_addr_t ifaceAddr;
    ifaceAddr.addr = interfaceAddress;

    if (!_ctx) {
        _ctx = new UdpContext;
        _ctx->ref();
    }
    if (!_ctx->connect(mcastAddr, port)) {
        return 0;
    }
    _ctx->setMulticastInterface(ifaceAddr);
    _ctx->setMulticastTTL(ttl);
    return 1;
}

int WiFiUDP::endPacket()
{
    if (!_ctx)
        return 0;

    _ctx->send();
    return 1;
}

size_t WiFiUDP::write(uint8_t byte)
{
    return write(&byte, 1);
}

size_t WiFiUDP::write(const uint8_t *buffer, size_t size)
{
    if (!_ctx)
        return 0;

    return _ctx->append(reinterpret_cast<const char*>(buffer), size);
}

int WiFiUDP::parsePacket()
{
    if (!_ctx)
        return 0;

    if (!_ctx->next()) {
        return 0;
    }

    return _ctx->getSize();
}

int WiFiUDP::read()
{
    if (!_ctx)
        return -1;

    return _ctx->read();
}

int WiFiUDP::read(unsigned char* buffer, size_t len)
{
    if (!_ctx)
        return 0;

    return _ctx->read(reinterpret_cast<char*>(buffer), len);
}

int WiFiUDP::peek()
{
    if (!_ctx)
        return 0;

    return _ctx->peek();
}

void WiFiUDP::flush()
{
    if (_ctx)
        _ctx->flush();
}

IPAddress WiFiUDP::remoteIP()
{
    if (!_ctx)
        return IPAddress(0U);

    return IPAddress(_ctx->getRemoteAddress());
}

uint16_t WiFiUDP::remotePort()
{
    if (!_ctx)
        return 0;

    return _ctx->getRemotePort();
}

IPAddress WiFiUDP::destinationIP()
{
    IPAddress addr;

    if (!_ctx)
        return addr;

    addr = _ctx->getDestAddress();
    return addr;
}

uint16_t WiFiUDP::localPort()
{
    if (!_ctx)
        return 0;

    return _ctx->getLocalPort();
}

void WiFiUDP::stopAll()
{
    for (WiFiUDP* it = _s_first; it; it = it->_next) {
        DEBUGV("%s %08x %08x\n", __func__, (uint32_t) it, (uint32_t) _s_first);
        it->stop();
    }
}

void WiFiUDP::stopAllExcept(WiFiUDP * exC) {
    for (WiFiUDP* it = _s_first; it; it = it->_next) {
        if (it->_ctx != exC->_ctx) {
            DEBUGV("%s %08x %08x\n", __func__, (uint32_t) it, (uint32_t) _s_first);
            it->stop();
        }
    }
}