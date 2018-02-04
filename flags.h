#ifndef _FLAGS_H_INCLUDED
#define _FLAGS_H_INCLUDED

#include <cstdint>
#include <cstdlib>
#include <iostream>

namespace testing {
enum class FlagType : uint8_t {
    kNone,
    kBool,
    kInt,
    kString
};

union FlagValue {
    FlagValue(bool b) : bval(b) {}
    FlagValue(int i) : ival(i) {}
    FlagValue(const char *s) : sval(s) {}

    bool bval;
    int ival;
    const char *sval;
};

class Flag;

class FlagList {
public:
    static bool ParseCommandLine(int argc, char *argv[]);
    static void Print(std::ostream& out);
private:
    friend class Flag;
    static void RegisterFlag(Flag *flag);
    static Flag *GetFlag(const char *name);

    static Flag *first_;
};

class Flag {
public:
    template<typename T>
    Flag(const char *name, 
         FlagType type, 
         T *ptr,
         T default_value, 
         const char *desc)
        : name_(name)
        , type_(type)
        , value_(new (ptr) FlagValue(default_value))
        , desc_(desc) {
        FlagList::RegisterFlag(this);
    }
private:
    friend class FlagList;

    void SetValue(const char *str) {
        switch (type_) {
        case FlagType::kBool:
            value_->bval = 
                (((str[0] == 'o') && (str[1] == 'n')))
                || ((str[0] == 't') && (str[1] == 'r') && (str[2] == 'u') && (str[3] == 'e'));
            break;
        case FlagType::kInt:
            value_->ival = atoi(str);
            break;
        case FlagType::kString:
            value_->sval = str;
            break;
        }
    }

    const char *name_;
    FlagType type_ = FlagType::kNone;
    FlagValue *value_;
    Flag *next_ = nullptr;
    const char *desc_ = nullptr;
};
}

#define DEFINE_FLAG(name, type, ftype, default_value, desc)                     \
type FLAG_##name;                                                               \
testing::Flag flag_##name(#name, ftype, &FLAG_##name, default_value, desc)

#define DEFINE_bool(name, default_value, desc)                                  \
 DEFINE_FLAG(name, bool, testing::FlagType::kBool, default_value, desc)

#define DEFINE_int(name, default_value, desc)                                   \
DEFINE_FLAG(name, int, testing::FlagType::kInt, default_value, desc)

#define DEFINE_string(name, default_value, desc)                                \
DEFINE_FLAG(name, const char*, testing::FlagType::kString, default_value, desc)

#endif // !_FLAGS_H_INCLUDED
