#ifndef __BSCRIPT_STRING_H_
#define __BSCRIPT_STRING_H_

#include "class.h"

class String : public Class
{
 private:
    std::string m_string;

 public:
    String();
    ~String();
};

#endif
