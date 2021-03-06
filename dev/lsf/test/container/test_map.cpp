// File:        test_map.cpp
// Description: ---
// Notes:       ---
// Author:      leoxiang <leoxiang727@qq.com>
// Revision:    2012-11-28 by leoxiang

#include <string>
#include <map>
#include "lsf/basic/unit_test.hpp"
#include "lsf/container/map.hpp"
#include "lsf/container/heap_mem.hpp"
#include "node.hpp"

using namespace std;
using namespace lsf::container;

#define CACHE_SIZE 5

LSF_TEST_CASE(easy_test) {
    Map<int, string, HeapMem> maps;
    maps.BindAndInitStorage(HeapMem(maps.CalcByteSize(CACHE_SIZE)));
    LSF_ASSERT(maps.IsBindStorage());
    LSF_ASSERT(maps.empty());

    // test default
    LSF_ASSERT(maps[1] == string(""));
    LSF_ASSERT(maps[2] == string(""));

    // test insert
    LSF_ASSERT(maps.Insert(3, "leo3"));
    LSF_ASSERT(maps.Insert(4, "leo4"));
    LSF_ASSERT(maps.Insert(5, "leo5"));
    LSF_ASSERT(maps.full());

    // test assign
    maps[1] = "leo1";
    maps[2] = "leo2";

    // test find
    LSF_ASSERT(maps.Find(1)->value == "leo1");
    LSF_ASSERT(maps.Find(2)->value == "leo2");
    LSF_ASSERT(maps.Find(3)->value == "leo3");
    LSF_ASSERT(maps.Find(4)->value == "leo4");
    LSF_ASSERT(maps.Find(5)->value == "leo5");

    // test iterator
    LSF_ASSERT((maps.begin() + 0)->value == "leo1");
    LSF_ASSERT((maps.begin() + 1)->value == "leo2");
    LSF_ASSERT((maps.begin() + 2)->value == "leo3");
    LSF_ASSERT((maps.begin() + 3)->value == "leo4");
    LSF_ASSERT((maps.begin() + 4)->value == "leo5");

    // test erase
    LSF_ASSERT(maps.Erase(1));
    LSF_ASSERT(maps.Erase(2));
    LSF_ASSERT(maps.Erase(3));
    LSF_ASSERT(maps.Erase(4));
    LSF_ASSERT(maps.Erase(5));
    LSF_ASSERT(maps.empty());
}

LSF_TEST_CASE(test_iter_erase) {
    Map<int, string, HeapMem> maps;
    maps.BindAndInitStorage(HeapMem(maps.CalcByteSize(CACHE_SIZE)));
    LSF_ASSERT(maps.IsBindStorage());
    LSF_ASSERT(maps.empty());

    // test insert
    LSF_ASSERT(maps.Insert(1, "leo1"));
    LSF_ASSERT(maps.Insert(2, "leo2"));
    LSF_ASSERT(maps.Insert(3, "leo3"));
    LSF_ASSERT(maps.Insert(4, "leo4"));
    LSF_ASSERT(maps.Insert(5, "leo5"));
    LSF_ASSERT(maps.full());

    // erase
    Map<int, string>::iterator iter = maps.begin();
    while (iter != maps.end()) {
        Map<int, string>::iterator cur_iter = iter++;
        LSF_ASSERT(maps.Erase(cur_iter->key));
    }
    LSF_ASSERT(maps.empty());

    // std::multimap<int, int, std::greater<int>> m;
    // m.emplace(100, 1);
    // m.emplace(100, 2);
    // m.emplace(100, 3);
    // m.emplace(200, 3);
    // m.emplace(200, 2);
    // m.emplace(200, 1);
    // m.emplace(100, 1);
    // m.emplace(100, 2);
    // m.emplace(100, 3);
    // for (auto& pair : m) {
    //     std::cout << pair.first << ":" << pair.second << std::endl;
    // }
}

int main(int argc, char **argv) { LSF_TEST_ALL(argc, argv); }

// vim:ts=4:sw=4:et:ft=cpp:
