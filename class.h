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
    Class* m_superClass;

    std::map<std::string, Function*> m_methods;
    int m_fieldStartId;
    std::vector<std::string> m_fields;

 public:
    Class(Class* superClass, std::string name);
    virtual ~Class();

    std::string getName() { return m_name; }
    Class* getSuperClass() { return m_superClass; }

    virtual size_t getFieldCount();

    void addField(std::string name);
    int getFieldId(std::string name);

    void addMethod(std::string name, Function* function);
    virtual Function* findMethod(std::string name);
};

#endif
