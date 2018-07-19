/* ***********************************************
Author :111qqz
mail: renkuanze@sensetime.com
Created Time :2018年07月19日 星期四 10时22分34秒
File Name :flow_graph.cpp
************************************************ */

#include <cstdio>
#include "tbb/flow_graph.h"
#include <graph.h>
using namespace tbb::flow;

struct body {
    std::string my_name;
    body(const char *name) : my_name(name) {}
    void operator()(continue_msg) const {
        printf("%s\n", my_name.c_str());
    }
};

int main() {
    graph g;

    broadcast_node< continue_msg > start(g);
  
    continue_node<continue_msg> a(g, body("AAA"));
    continue_node<continue_msg> b(g, body("BBBBB"));
    continue_node<continue_msg> c(g, body("C"));
    continue_node<continue_msg> d(g, body("D"));
    continue_node<continue_msg> e(g, body("E"));

    make_edge(start, a);
    make_edge(start, b);
    make_edge(a, c);
    make_edge(b, c);
    make_edge(c, d);
    make_edge(a, e);
	g.Compile()
    for (int i = 0; i < 3; ++i) {
        start.try_put(continue_msg());
        g.wait_for_all();
    }

    return 0;
}

