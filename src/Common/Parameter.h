#include "UnityIncludes.h"

enum ParamType
{
    Bool,
    Int,
    Float,
    Str,
    P2D,
    P3D,
    TypeColor,
    TypeEnum,
    ParamTypeMax
};

enum ParamTag
{
    TagNone = 0,
    TagConfig = 1,
    TagFeedback = 2,
    TagNameMax
};

struct ParamRange
{
    float vMin = 0;
    float vMax = 0;
    float vMin2 = 0;
    float vMax2 = 0;
    float vMin3 = 0;
    float vMax3 = 0;
};

ParamRange defaultRange{0, 1};

const std::string paramTypeNames[ParamTypeMax]{"I", "b", "i", "f", "s", "ff", "fff", "r"};
// const std::string paramTagNames[TagNameMax]{"", "config", "feedback"};

struct ParamInfo
{
    void *ptr = nullptr;
    std::string name;
    ParamType type;
    uint8_t tags; // bitmask of ParamTag
    ParamRange* range = nullptr;
    std::string* enumOptions = nullptr;
    uint8_t numEnumOptions = 0;

    std::string getTypeString() const
    {
        return paramTypeNames[type];
    }

    void setTag(ParamTag tag, bool enable)
    {
        if (enable)
            tags |= (1 << tag);
        else
            tags &= ~(1 << tag);
    }

    bool hasTag(ParamTag tag) const
    {
        return (tags & (1 << tag)) != 0;
    }

    void setOptions(const std::string* options, uint8_t numOptions)
    {
        enumOptions = (std::string*)options;
        numEnumOptions = numOptions;
    }

    void setRange(ParamRange* r)
    {
        range = r;
    }
};


struct Trigger
{
    std::string name;
    std::function<void(void)> func;
};