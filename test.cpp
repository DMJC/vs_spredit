#include "spr_parser.h"
#include <cassert>
#include <iostream>

int main() {
    // ends_with
    assert(ends_with("file.ani", ".ani"));
    assert(!ends_with("file.spr", ".ani"));

    // is_integer
    assert(is_integer("123"));
    assert(is_integer("-45"));
    assert(!is_integer("12a"));
    assert(!is_integer("+"));

    // is_double
    assert(is_double("12.3"));
    assert(is_double("-0.5"));
    assert(is_double("3"));
    assert(!is_double("abc"));

    // parse_cropping_params
    double mins=0, mint=0, maxs=0, maxt=0;
    bool ok = parse_cropping_params("mins=0.1,mint=0.2,maxs=0.8,maxt=0.9", mins, mint, maxs, maxt);
    assert(ok);
    assert(mins == 0.1);
    assert(mint == 0.2);
    assert(maxs == 0.8);
    assert(maxt == 0.9);

    ok = parse_cropping_params("mins=0.1,mint=foo,maxs=1,maxt=0.9", mins, mint, maxs, maxt);
    assert(!ok);

    std::cout << "All tests passed" << std::endl;
    return 0;
}
