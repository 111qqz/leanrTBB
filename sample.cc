#include "graph.h"
#include <thread>
#include <functional>
#include <chrono>
#include <future>
#include <utility>
#include <cmath>
#include <tbb/task_scheduler_init.h>
#include "utils.h"

using std::tuple;
using std::make_tuple;
using std::shared_ptr;
using namespace std::chrono_literals;

class GraphTest {
 public:
  explicit GraphTest(int n_token): g_(n_token), sync_(n_token == 1) {}

  void Init() {
    using namespace std::chrono_literals;

    auto input = g_.Function([](float x) { return x; });
    auto f0 = g_.Function(input, [](float x) { return x; });
    auto f1 = g_.Function(input, [](float x) { return x * x; });

    auto queue = g_.Queue(input);

    auto t2_1 = g_.Timer(100ms, [](){ return -1.f; });
    auto f2_1 = g_.Function(queue, t2_1, [vec=vector<float>{}, batch_size=sync_?1:4, batch_id=0LL](float x) mutable {
      if (x >= 0)
        vec.push_back(x);
      if (vec.size() == batch_size || (x < 0 && !vec.empty())) {
        LOG_IF(INFO, x < 0) << "Triggered by timer.";
        for (auto& y: vec)
          y *= y * y;
        std::this_thread::sleep_for(50ms);
        vector<XArgs> rets;
        for (auto& y: vec)
          rets.emplace_back(y);
        vec.clear();
        return make_tuple(rets, PUSH(batch_id++));
      }
      return make_tuple(vector<XArgs>{}, HOLD(batch_id));
    });

    auto t2_2 = g_.Timer(100ms, [](){ return -1.f; });
    auto f2_2 = g_.Function(queue, t2_2, [vec=vector<float>{}, batch_size=sync_?1:8, batch_id=0LL](float x) mutable {
      if (x >= 0)
        vec.push_back(x);
      if (vec.size() == batch_size || (x < 0 && !vec.empty())) {
        LOG_IF(INFO, x < 0) << "Triggered by timer.";
        for (auto& y: vec)
          y *= y * y;
        std::this_thread::sleep_for(50ms);
        vector<XArgs> rets;
        for (auto& y: vec)
          rets.push_back(make_xargs(y));
        vec.clear();
        return make_tuple(rets, PUSH(batch_id++));
      }
      return make_tuple(vector<XArgs>{}, HOLD(batch_id));
    });

    auto seq = g_.Sequencer(f2_1, f2_2);

    auto concat = g_.Concat(f0, f1, seq);
    auto output = g_.Function(concat, [](float x, float y, float z) {
      LOG(INFO) << x << " " << y << " " << z;
    });

    XArgs concat_init(vector<float>{});
    auto custom_concat = g_.ConcatUniform({f0, f1, seq}, concat_init, [](float x, vector<float>& a) {
      a.push_back(x);
    });

    auto out = g_.Function(custom_concat, [](vector<float> x) {
//      LOG(INFO) << "\t" << x[0] << " " << x[1] << " " << x[2];
    });

    g_.Compile();
  }

  void InitSubgraphTest() {
    auto a = g_.Function([](float x) { return x + 1; });

    subgraph_.reset(new Graph(1024));
    auto sa = subgraph_->Function([](float x) { return x + 10; });
    subgraph_->Compile();

    auto b = g_.Subgraph(a, subgraph_.get());
    auto c = g_.Function(b, [](float x) { LOG(INFO) << x; });
    g_.Compile();
  }


  void InitConcurrencyTest() {
    auto a = g_.Function({}, [](float x) {
      LOG(INFO) << "Sleeping";
      std::this_thread::sleep_for(0.001s);
      LOG(INFO) << "Waking Up";
    }, 10);

    g_.Compile();

    for (int i = 0; i < 10; ++i) {
      g_.Enqueue((float)i);
    }
  }

  void GenerateIgnoreTest() {
    //where is the second broadcast node?

    auto input = g_.Broadcast();


    auto x = g_.Function(input, [](int x) {
      vector<XArgs> msgs;
      const int N = 100;
      for (int i = 0; i < N; ++i) {
        msgs.emplace_back(i+x, N);
      }
      // return vector<XArgs> with GENERATE will generate N messages with items stored in the vector
      // seq_id will be sequential
      return make_tuple(msgs, GENERATE);
    });

    // not necessary, just demonstrating correctness of seq_id
    auto seq_x = g_.Sequencer(x);

    auto y = g_.Function(seq_x, [sum = 0, i = 0](int x, int N) mutable {
      sum += x;
      if (++i == N) {
        auto tmp = sum;
        sum = i = 0;
        // return scalar (not vector<XArgs>) with GENERATE will generate a single message,
        // seq_id will be sequential despite the use of IGNORE below
        return XArgs(tmp, GENERATE);
      }
      return XArgs(IGNORE);
    });

    // not necessary, just demonstrating correctness of seq_id
//    auto seq_y = g_.Sequencer(y);

    auto output = g_.Function(y, [](int x) {
      LOG(INFO) << x;
    });

    g_.Compile();
    g_.Enqueue(0);
    g_.Enqueue(1);
    g_.Enqueue(2);
    g_.Enqueue(3);
    g_.Enqueue(4);
    g_.Wait();
  }


  template <typename...Args>
  std::vector<XArgs> execute(Args...args) {
    return g_.Execute(args...);
  }

  template <typename...Args>
  bool enqueue(Args...args) {
    return g_.Enqueue(args...);

//     return g_.Feed(Node(0), args...);
  }

  void Wait() { g_.Wait(); }
 private:
  std::shared_ptr<Graph> subgraph_;
  Graph g_;
  bool sync_;
};

// NOTE: do NOT delete this
void TestXValue() {
  LOG(INFO) << ">>> " << __PRETTY_FUNCTION__;
  XValue x(1);
  XValue y(x);
  auto z = y;

  LOG(INFO) << y.AsType<int>();
  LOG(INFO) << "<<< " << __PRETTY_FUNCTION__;
}


void TestXArgs() {
  LOG(INFO) << ">>> " << __PRETTY_FUNCTION__;
  XArgs msg(1, 2, 3, 4, 5.f, string("testtest"));
  LOG(INFO) << msg[0].AsType<int>();
  LOG(INFO) << msg[1].AsType<int>();
  LOG(INFO) << msg[2].AsType<int>();
  LOG(INFO) << msg[3].AsType<int>();
  LOG(INFO) << msg[4].AsType<float>();
  LOG(INFO) << msg[5].AsType<string>();
  LOG(INFO) << "<<< " << __PRETTY_FUNCTION__;
}


void TestRefArgs() {
  LOG(INFO) << ">>> " << __PRETTY_FUNCTION__;
  XArgs msg(1, 2, 3);
  auto f = PackFunction([](int& x, int& y, int& z){
    ++x;
    ++y;
    ++z;
    return 0;
  });
  f(msg);
  LOG(INFO) << msg[0].AsType<int>() << " " << msg[1].AsType<int>() << " " << msg[2].AsType<int>();
  LOG(INFO) << "<<< " << __PRETTY_FUNCTION__;
};


int main(int argc, char* argv[]) {

  FLAGS_logtostderr = true;
  google::InitGoogleLogging(argv[0]);
  google::InstallFailureSignalHandler();

  tbb::task_scheduler_init init(32);

  // TestXValue();
  // TestXArgs();
  // TestRefArgs();

  using namespace std::chrono_literals;

  // n_token == 1 for sync mode
  int n_token = 30;
  GraphTest test(n_token);

   test.InitConcurrencyTest();
  //test.GenerateIgnoreTest();
  return 0;


//  test.Init();
//  test.InitSubgraphTest();

  if (n_token > 1) { // async
    for (int i = 0; i <= 100; ++i) {
      while (!test.enqueue(static_cast<float>(i))) {
        std::this_thread::sleep_for(std::chrono::microseconds(1));
      }
    }
  } else { // sync
    for (int i = 0; i <= 100; ++i) {
      test.enqueue(static_cast<float>(i));
      test.Wait();
    }
  }

//  test.Wait();
  return 0;
}


