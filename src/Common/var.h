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
    std::string s;
    bool shouldClear = false;

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

    var(const std::string &v)
    {
        type = 's';
        s = v;
        size = v.length();
    }

    var(const char *v)
    {
        type = 's';
        s = std::string(v);
        size = s.length();
    }

    var(const uint8_t *ptr, uint8_t dataSize, bool ownData = false)
    {
        type = 'p';
        value.ptr = ptr;
        size = dataSize;
        shouldClear = ownData;
    }

    ~var()
    {
        if (shouldClear && type == 'p')
           free((void*)value.ptr);
    }

    operator bool() const { return boolValue(); }
    operator int() const { return intValue(); }
    operator float() const { return floatValue(); }
    operator std::string() const { return stringValue(); }
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
            return (bool)(s == "true" || s == "1" || s == "yes" || s == "on");

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
            return std::stoi(s);
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
            return std::stof(s);
        case 'p':
            return (float)*value.ptr;
        }

        return 0;
    }

    std::string stringValue() const
    {
        switch (type)
        {
        case 'b':
            return std::string(value.b?"true":"false");
        case 'i':
            return std::to_string(value.i);
        case 'f':
            return std::to_string(value.f);
        case 's':
            return s;
        case 'p':
            std::string str = "[";
            for (int i = 0; i < size; i++)
            {
                str += std::to_string(value.ptr[i]);
                if (i < size - 1)
                    str += ",";
            }
            return str + "]";
        }
        return "";
    }

    uint8_t getSize() const
    {
        switch (type)
        {
        case 'b':
            return sizeof(bool);
        case 'i':
            return sizeof(int);
        case 'f':
            return sizeof(float);
        case 's':
            return s.length();
        case 'p':
            return size;
        }

        return 0;
    }

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

    var &operator=(const std::string &v)
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
            s = std::string(v);

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
