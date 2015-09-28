
#include <b0rk/value.h>
#include <b0rk/object.h>

using namespace std;

string Value::toString()
{
    char buffer[128];
    switch (type)
    {
        case VALUE_VARIABLE:
            snprintf(buffer, 128, "{VARIABLE:%p}", variable);
            return string(buffer);

        case VALUE_OBJECT:
            if (object != NULL)
            {
                if (object->getClass() != NULL)
                {
                    snprintf(buffer, 128, "{OBJECT:%s:%p}", object->getClass()->getName().c_str(), object);
                }
                else
                {
                    snprintf(buffer, 128, "{OBJECT:UNKNOWN CLASS?:%p}", object);
                }
            }
            else
            {
                snprintf(buffer, 128, "{OBJECT:NULL}");
            }
            return string(buffer);

        case VALUE_POINTER:
            snprintf(buffer, 128, "{POINTER:%p}", pointer);
            return string(buffer);

        case VALUE_INTEGER:
            snprintf(buffer, 128, "%lld", i);
            return string(buffer);

        case VALUE_DOUBLE:
            snprintf(buffer, 128, "%0.2f", d);
            return string(buffer);

        case VALUE_FRAME:
            snprintf(buffer, 128, "{FRAME:%p}", pointer);
            return string(buffer);

        case VALUE_VOID:
            return string("{VOID}");

        case VALUE_UNKNOWN:
        default:
            return string("{UNKNOWN}");
    }
}

