struct var
{
    char type;
    union
    {
        bool b;
        int i;
        float f;
        const uint8_t *ptr;
    } value;

    uint8_t size;
    String s;

    var() { type = '?'; }

    var(bool v)
    {
        type = 'b';
        value.b = v;
        size = sizeof(bool);
    };
    var(int v)
    {
        type = 'i';
        value.i = v;
        size = sizeof(int);
    };
    var(float v)
    {
        type = 'f';
        value.f = v;
        size = sizeof(float);
    };

    var(const String &v)
    {
        type = 's';
        s = v;
        size = v.length();
    }

    var(const char *v)
    {
        type = 's';
        s = String(v);
        size = s.length();
    }

    var(const uint8_t *ptr, uint8_t dataSize)
    {
        type = 'p';
        value.ptr = ptr;
        size = dataSize;
    }

    operator bool() const { return boolValue(); }
    operator int() const { return intValue(); }
    operator float() const { return floatValue(); }
    operator String() const { return stringValue(); }
    operator char *() const { return (char *)s.c_str(); }
    operator const uint8_t *() const { return value.ptr; }

    bool isVoid() const { return type == '?'; }

    bool boolValue() const
    {
        switch (type)
        {
        case 'b':
            return value.b;
        case 'i':
            return (bool)value.i;
        case 'f':
            return (bool)(int)value.f;

        case 's':
            return (bool)s.toInt();

        case 'p':
            return value.ptr != NULL;
        }
        return 0;
    }

    int intValue() const
    {
        switch (type)
        {
        case 'b':
            return (int)value.b;

        case 'i':
            return value.i;
        case 'f':
            return (int)value.f;
        case 's':
            return s.toInt();
        case 'p':
            return (int)*value.ptr;
        }
        return 0;
    }

    float floatValue() const
    {
        switch (type)
        {
        case 'b':
            return (float)value.b;
        case 'i':
            return (float)value.i;
        case 'f':
            return value.f;
        case 's':
            return s.toFloat();
        case 'p':
            return (float)*value.ptr;
        }

        return 0;
    }

    String stringValue() const
    {
        switch (type)
        {
        case 'b':
            return String((int)value.b);
        case 'i':
            return String(value.i);
        case 'f':
            return String(value.f);
        case 's':
            return s;
        case 'p':
            return "[ptr]";
        }
        return "";
    }

    bool getPtrBool(uint8_t offset) const { return (bool)value.ptr[offset]; }
    int getPtrInt(uint8_t offset) const { return (int)value.ptr[offset]; }
    float getPtrFloat(uint8_t offset) const { return (float)value.ptr[offset]; }
    String getPtrString(uint8_t offset, uint8_t len) const { return String((char *)value.ptr + offset, len); }

    var &operator=(const bool v)
    {
        if (type == '?')
            type = 'b';
        if (type == 'b')
            value.b = v;
        return *this;
    }

    var &operator=(const int v)
    {
        if (type == '?')
            type = 'i';
        if (type == 'i')
            value.i = v;
        return *this;
    }

    var &operator=(const float v)
    {
        if (type == '?')
            type = 'f';
        if (type == 'f')
            value.f = v;
        return *this;
    }

    var &operator=(const String &v)
    {
        if (type == '?')
            type = 's';

        if (type == 's')
            s = v;

        // Serial.println("= string : "+stringValue());
        return *this;
    }

    var &operator=(const char *const v)
    {
        if (type == '?')
            type = 's';

        if (type == 's')
            s = String(v);

        return *this;
    }

    bool operator==(const var &other) const
    {
        switch (type)
        {
        case 'b':
            return boolValue() == other.boolValue();
        case 'i':
            return intValue() == other.intValue();
        case 'f':
            return floatValue() == other.floatValue();
        case 's':
            return stringValue() == other.stringValue();
        case 'p':
            return value.ptr == other.value.ptr;
        }

        return false;
    }
};
