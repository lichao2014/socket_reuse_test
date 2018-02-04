#include "flags.h"

#include <cstring>
#include <iomanip>

using namespace testing;

// static 
Flag *FlagList::first_ = nullptr;

// static 
bool FlagList::ParseCommandLine(int argc, char *argv[]) {
    Flag *flag = nullptr;
    for (int i = 1; i < argc; ++i) {
        // value
        if (flag) {
            if ('-' == argv[i][0]) {
                if (FlagType::kBool != flag->type_) {
                    return false;
                }

                flag->value_->bval = true;
            } else {
                flag->SetValue(argv[i]);
                flag = nullptr;
                continue;
            }
        }

        // name
        if ('-' != argv[i][0]) {
            return false;
        }

        flag = GetFlag(argv[i] + 1);
        if (!flag) {
            return false;
        }
    }

    // bool flag maybe be without value
    if (flag) {
        if (FlagType::kBool != flag->type_) {
            return false;
        }

        flag->value_->bval = true;
    }

    return true;
}

// static 
void FlagList::Print(std::ostream& out) {
    Flag *flag = first_;
    while (flag) {
        out << std::setw(8) << '-' << flag->name_
            << std::setw(16) << flag->desc_ << std::endl;

        flag = flag->next_;
    }
}

// static 
void FlagList::RegisterFlag(Flag *flag) {
    // FILO
    flag->next_ = first_;
    first_ = flag;
}

// static 
Flag *FlagList::GetFlag(const char *name) {
    Flag *flag = first_;
    while (flag) {
        if (0 == strcmp(name, flag->name_)) {
            return flag;
        }

        flag = flag->next_;
    }

    return nullptr;
}
