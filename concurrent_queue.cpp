/* ***********************************************
Author :111qqz
mail: renkuanze@sensetime.com
Created Time :2018年08月23日 星期四 11时12分13秒
File Name :concurrent_queue.cpp
************************************************ */
#include "tbb/concurrent_queue.h"
#include <iostream>

using namespace std;
using namespace tbb;

int main() {
    concurrent_queue<int> queue;
    for( int i=0; i<10; ++i )
        queue.push(i);
    typedef concurrent_queue<int>::iterator iter;
    for( iter i(queue.unsafe_begin()); i!=queue.unsafe_end(); ++i )
        cout << *i << " ";
    cout << endl;
    return 0;
}
