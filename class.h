#ifndef __BSCRIPT_CLASS_H_
#define __BSCRIPT_CLASS_H_

#include "function.h"

#include <map>
#include <string>
#include <vector>

class Function;

class Class
{
 private:
    std::string m_name;
    std::map<std::string, Function*> m_methods;
    std::vector<std::string> m_fields;

 public:
    Class(std::string name);
    virtual ~Class();

    std::string getName() { return m_name; }

    virtual size_t getValueCount();

    void addField(std::string name);
    int getFieldId(std::string name);

    void addMethod(std::string name, Function* function);
    virtual Function* findMethod(std::string name);
};

#endif
