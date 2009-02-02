/*
    LinKNX KNX home automation platform
    Copyright (C) 2007-2009 Jean-François Meessen <linknx@ouaye.net>
 
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
 
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
 
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef IOPORT_H
#define IOPORT_H

#include "config.h"
#include "logger.h"
#include "threads.h"
#include <string>
#include <memory>
#include "ticpp.h"
#include "ruleserver.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>



class IOPortListener
{
public:
    virtual void onDataReceived(const uint8_t* buf, int len) = 0;
};

class RxThread;
class IOPort;

class IOPortManager
{
public:
    static IOPortManager* instance();
    static void reset()
    {
        if (instance_m)
            delete instance_m;
        instance_m = 0;
    };
    void addPort(IOPort* conn);
    void removePort(IOPort* conn);

    IOPort* getPort(const std::string& id);

    virtual void importXml(ticpp::Element* pConfig);
    virtual void exportXml(ticpp::Element* pConfig);

//    virtual void exportObjectValues(ticpp::Element* pObjects);

private:
    IOPortManager();
    virtual ~IOPortManager();

    typedef std::pair<std::string ,IOPort*> IOPortPair_t;
    typedef std::map<std::string ,IOPort*> IOPortMap_t;
    IOPortMap_t portMap_m;
    static IOPortManager* instance_m;
};

class IOPort
{
public:
    enum Direction
    {
        In,
        Out,
        InOut
    };

    IOPort();
    virtual ~IOPort();

    static IOPort* create(ticpp::Element* pConfig);
    static IOPort* create(const std::string& type);

    virtual void importXml(ticpp::Element* pConfig);
    virtual void exportXml(ticpp::Element* pConfig);

    void setID(const char* id) { id_m = id; };
    const char* getID() { return id_m.c_str(); };

    void addListener(IOPortListener *l); // { if (rxThread_m) rxThread_m->addListener(l); };
    bool removeListener(IOPortListener *l); // { if (rxThread_m) return (rxThread_m->removeListener(l)) else return false; };
    virtual void send(const uint8_t* buf, int len) = 0;
    virtual int get(uint8_t* buf, int len) = 0;

private:
    std::auto_ptr<RxThread> rxThread_m;
    std::string id_m;
    std::string url_m;
    
    Direction dir_m;

    static Logger& logger_m;
};

class RxThread : public Thread
{
public:
    RxThread(IOPort *port);
    virtual ~RxThread();

    void startPort() { isRunning_m = true; Start(); };
    void stopPort() { isRunning_m = false; Stop(); };

    void addListener(IOPortListener *listener);
    bool removeListener(IOPortListener *listener);

private:
    IOPort *port_m;
    bool isRunning_m;
    pth_event_t stop_m;
    IOPortListener *listener_m;

    void Run (pth_sem_t * stop);
    static Logger& logger_m;
};

class UdpIOPort : public IOPort
{
public:
    UdpIOPort();
    virtual ~UdpIOPort();

    virtual void importXml(ticpp::Element* pConfig);
    virtual void exportXml(ticpp::Element* pConfig);

    void send(const uint8_t* buf, int len);
    int get(uint8_t* buf, int len);

private:
    std::string host_m;
    int sockfd_m;
    int port_m;
    struct sockaddr_in addr_m;
    static Logger& logger_m;
};

class TxAction : public Action
{
public:
    TxAction();
    virtual ~TxAction();

    virtual void importXml(ticpp::Element* pConfig);
    virtual void exportXml(ticpp::Element* pConfig);

private:
    virtual void Run (pth_sem_t * stop);

    std::string data_m;
    IOPort* port_m;
};

class RxCondition : public Condition, public IOPortListener
{
public:
    RxCondition(ChangeListener* cl);
    virtual ~RxCondition();

    virtual bool evaluate();
    virtual void importXml(ticpp::Element* pConfig);
    virtual void exportXml(ticpp::Element* pConfig);

    virtual void onDataReceived(const uint8_t* buf, int len);

private:
    IOPort* port_m;
    std::string exp_m;
    bool value_m;
    ChangeListener* cl_m;
};


#endif