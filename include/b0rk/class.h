#ifndef __BSCRIPT_CLASS_H_
#define __BSCRIPT_CLASS_H_

#include <b0rk/function.h>
#include <b0rk/value.h>

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
    std::vector<std::string> m_staticFields;

    Value* m_staticValues;

 public:
    Class(Class* superClass, std::string name);
    virtual ~Class();

    std::string getName() { return m_name; }
    Class* getSuperClass() { return m_superClass; }

    virtual size_t getFieldCount();
    virtual size_t getStaticFieldCount();

    void addField(std::string name);
    int getFieldId(std::string name);

    void addStaticField(std::string name);
    int getStaticFieldId(std::string name);
    Value getStaticField(int slot) { return m_staticValues[slot]; }
    void setStaticField(int slot, Value v) { m_staticValues[slot] = v; }
    void initStaticFields();
    std::vector<std::string>& getStaticFields();

    void addMethod(std::string name, Function* function);
    virtual Function* findMethod(std::string name);
};

#endif
