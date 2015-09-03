
#include "object.h"

Object::Object(Class* clazz)
{
    m_class = clazz;

    m_valueCount = clazz->getValueCount();
    m_values = new Value[m_valueCount];
}

Object::~Object()
{
    delete m_values;
}

