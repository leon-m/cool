

#include <iostream>
#include <thread>
#include <chrono>
#include <vector>

#include <cool2/async.h>
#include <cool/cool.h>

using namespace cool::basis;

void big_test()
{
  auto r = std::make_shared<cool::async::runner>();
  std::vector<cool::async::task<cool::async::impl::tag::simple, cool::async::runner, void, void>> tasks;
  const uint64_t ntasks = 2000000;
  vow<void> v_;
  auto a_ = v_.get_aim();

  int step = 0;

  for (int i = 0; i < ntasks; ++i)
    tasks.push_back(
      cool::async::factory::create(
          r
        , [i, &step] (const std::shared_ptr<cool::async::runner>&)
          {
           if (i == step)
             ++step;
          }
      ));
  tasks.push_back(
    cool::async::factory::create(
        r
      , [&v_] (const std::shared_ptr<cool::async::runner>&)
        {
          v_.set();
        }
   ));

  auto t_start = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < tasks.size(); ++i)
    tasks[i].run();

  try
  {
    a_.get(std::chrono::milliseconds(60000));
    auto t_stop = std::chrono::high_resolution_clock::now();
  
    uint64_t period = std::chrono::duration_cast<std::chrono::milliseconds>(t_stop - t_start).count();
    uint64_t rate = (ntasks * 1000) / period;
    std::cout << "Elapsed:   " << period << " milliseconds\n";
    std::cout << "Task rate: " << rate << " tasks/sec\n";
    if (ntasks != step)
      std::cerr << "ERROR: Only " << step << " tasks of " << ntasks << " were run\n";
  }
  catch (const cool::exception::timeout&  e)
  {
    std::cerr << "ERROR: timeout occured: " << e.what() << std::endl;
  }
}

int main(int argc, char* argv[])
{
  big_test();
}