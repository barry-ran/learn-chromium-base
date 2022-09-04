#include <iostream>

#include "base/strings/string_number_conversions.h"

int main(int argc, char *argv[]) {
    std::cout << "hello:" << base::NumberToString(111) << std::endl;
    return 0;
}