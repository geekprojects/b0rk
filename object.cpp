
#include "object.h"

Object::Object(Class* clazz)
{
    m_class = clazz;

    m_values = new Value[clazz->getValueCount()];
}

Object::~Object()
{
    delete m_values;
}

