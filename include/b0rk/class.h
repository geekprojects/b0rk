/*
 * b0rk - The b0rk Embeddable Runtime Environment
 * Copyright (C) 2015, 2016 GeekProjects.com
 *
 * This file is part of b0rk.
 *
 * b0rk is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * b0rk is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with b0rk.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __BSCRIPT_CLASS_H_
#define __BSCRIPT_CLASS_H_

#include <b0rk/function.h>
#include <b0rk/value.h>

#include <map>
#include <string>
#include <vector>

namespace b0rk
{

class Function;
class Class;
class Context;

enum ClassState
{
    INIT,
    PARSING,
    COMPLETE
};

class Class
{
 private:
    std::wstring m_name;
    ClassState m_state;
    Class* m_superClass;

    std::map<std::wstring, Function*> m_methods;

    int m_fieldStartId;
    std::vector<std::wstring> m_fields;

    int m_staticFieldStartId;
    std::vector<std::wstring> m_staticFields;
    std::vector<Value> m_staticValues;
    //Value* m_staticValues;

 public:
    Class(Class* superClass, std::wstring name);
    virtual ~Class();

    std::wstring getName() { return m_name; }
    ClassState getState() { return m_state; }
    void setState(ClassState state) { m_state = state; }
    void setSuperClass(Class* sc) { m_superClass = sc; }
    Class* getSuperClass() { return m_superClass; }

    virtual size_t getFieldCount();
    virtual size_t getStaticFieldCount();

    void addField(std::string name);
    void addField(std::wstring name);
    int getFieldId(std::wstring name);

    int addStaticField(std::string name);
    int addStaticField(std::wstring name);
    int getStaticFieldId(std::wstring name);
    Value getStaticField(int slot) { return m_staticValues[slot - m_staticFieldStartId]; }
    void setStaticField(int slot, Value v) { m_staticValues[slot - m_staticFieldStartId] = v; }
    //void initStaticFields();
    std::vector<std::wstring>& getStaticFields();

    void addMethod(std::string name, Function* function);
    void addMethod(std::wstring name, Function* function);
    virtual Function* findMethod(std::wstring name);

    virtual bool deleteOnExit() { return true; }
};

};

#endif
