#ifndef SESSION_HPP
#define SESSION_HPP

#include <string>
#include <map>
#include <sstream>
#include <ctime>

class SessionManager
{
private:
    std::map<std::string, std::map<std::string, std::string> > _store;
    unsigned long _counter;

public:
    SessionManager() : _counter(0) {}

    bool has(const std::string &id) const
    {
        return _store.find(id) != _store.end();
    }

    std::string create()
    {
        std::ostringstream oss;
        oss << "sid-" << static_cast<long>(std::time(NULL)) << "-" << _counter++;
        std::string id = oss.str();
        _store[id];
        return id;
    }

    std::map<std::string, std::string> &data(const std::string &id)
    {
        return _store[id];
    }
};

#endif
