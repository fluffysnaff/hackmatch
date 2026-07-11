#include "build_info.h"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>

int main()
{
    using hackmatch::parse_steam_build_id;
    assert(parse_steam_build_id(R"("AppState" { "buildid" "23904900" })") == "23904900");
    assert(parse_steam_build_id("\n\t\"buildid\"\t\"12345\"\n") == "12345");
    assert(!parse_steam_build_id(R"("AppState" { "name" "Redmatch 2" })"));
    assert(!parse_steam_build_id(R"("buildid" "")"));
    assert(!parse_steam_build_id(R"("buildid" "latest")"));
}
