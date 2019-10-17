#include <Fw/Types/StringType.hpp>
#include <Fw/Types/BasicTypes.hpp>
#include <Os/QueueString.hpp>
#include <Fw/Types/Assert.hpp>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

namespace Os {

    QueueString::QueueString(const char* src) : StringBase()  {
        this->copyBuff(src,sizeof(this->m_buf));
    }

    QueueString::QueueString(const StringBase& src) : StringBase()  {
        this->copyBuff(src.toChar(),sizeof(this->m_buf));
    }

    QueueString::QueueString(const QueueString& src)  : StringBase() {
        this->copyBuff(src.m_buf,sizeof(this->m_buf));
    }

    QueueString::QueueString(void) : StringBase()  {
        this->m_buf[0] = 0;
    }

    QueueString::~QueueString(void) {
    }

    const QueueString& QueueString::operator=(const QueueString& other) {
        this->copyBuff(other.m_buf,this->getCapacity());
        return *this;
    }


    NATIVE_UINT_TYPE QueueString::length(void) const {
        if(sizeof(this->m_buf)){
            return strlen(this->m_buf);
        }
        else{
            return 0;
        }
    }

    const char* QueueString::toChar(void) const {
        return this->m_buf;
    }

    void QueueString::copyBuff(const char* buff, NATIVE_UINT_TYPE size) {
        FW_ASSERT(buff);
        // check for self copy
        if (buff != this->m_buf) {
            (void)strncpy(this->m_buf,buff,size);
            // NULL terminate
            this->terminate(sizeof(this->m_buf));
        }
    }
    
    NATIVE_UINT_TYPE QueueString::getCapacity(void) const {
        return FW_QUEUE_NAME_MAX_SIZE;
    }

    void QueueString::terminate(NATIVE_UINT_TYPE size) {
        // null terminate the string
        this->m_buf[size < sizeof(this->m_buf)?size:sizeof(this->m_buf)-1] = 0;
    }
    
}
