// See LICENSE for license details.

#ifndef _RISCV_CACHE_SIM_H
#define _RISCV_CACHE_SIM_H

#include "memtracer.h"
#include "common.h"
#include <cstring>
#include <string>
#include <map>
#include <cstdint>
#include <deque>
#include <vector>

class lfsr_t
{
 public:
  lfsr_t() : reg(1) {}
  lfsr_t(const lfsr_t& lfsr) : reg(lfsr.reg) {}
  uint32_t next() { return reg = (reg>>1)^(-(reg&1) & 0xd0000001); }
 private:
  uint32_t reg;
};

class cache_sim_t
{
  // size_t in 64-bit computer is an unsigned 64-bit integer
  // uint64_t is an unsigned 64-bit integer
  // they are usually unsigned long long
 public:
  cache_sim_t(size_t sets, size_t ways, size_t linesz, const char* name); // constructor
  cache_sim_t(const cache_sim_t& rhs);// copy constructor
  virtual ~cache_sim_t(); // decontructor

  void access(uint64_t addr, size_t bytes, bool store);// access cache
  void clean_invalidate(uint64_t addr, size_t bytes, bool clean, bool inval);// clean or invalidate cache
  void print_stats();// print datas in cache
  void set_miss_handler(cache_sim_t* mh) { miss_handler = mh; }
  void set_log(bool _log) { log = _log; }// 啟用設定日誌

  // produce a n-way set associative cache or a fully associatitve cache
  static cache_sim_t* construct(const char* config, const char* name);

 protected:
  // 2 constants for dirty bit and valid bit
  static const uint64_t VALID = 1ULL << 63;// 1000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000
  static const uint64_t DIRTY = 1ULL << 62;// 0100 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000

  /* 2 methods below are important!!!!*/
  virtual uint64_t* check_tag(uint64_t addr);
  virtual uint64_t victimize(uint64_t addr);

  lfsr_t lfsr;// a instance of LFSR replacement policy class
  cache_sim_t* miss_handler;// IDK

  size_t sets;// #sets 表示有幾條set在一個cache裡面
  size_t ways;// #ways 表示一個set裡面有幾個block
  size_t linesz;// block size 當資料在不同層級流動時的資料單位大小
  size_t idx_shift;// 表示在一個addr中idx field的位置

  uint64_t* tags;// 儲存放入cache的data addr的tag值的array，操作cache時會需要使用這個array以檢查cache中是否存在指定的block
  
  uint64_t read_accesses;
  uint64_t read_misses;
  uint64_t bytes_read;
  uint64_t write_accesses;
  uint64_t write_misses;
  uint64_t bytes_written;
  uint64_t writebacks;

  std::string name;
  bool log;

  void init();

  /*
   * FIFO data structure, queue
   * one vector represents one set
   * one queue contains blocks of a set
   * 這個應有同tags[]的功能
   */
  std::vector<std::deque<uint64_t>> fifo_queues;
};

class fa_cache_sim_t : public cache_sim_t
{
 public:
  fa_cache_sim_t(size_t ways, size_t linesz, const char* name);
  uint64_t* check_tag(uint64_t addr);
  uint64_t victimize(uint64_t addr);
 private:
  static bool cmp(uint64_t a, uint64_t b);
  std::map<uint64_t, uint64_t> tags;
};

class cache_memtracer_t : public memtracer_t
{
 public:
  cache_memtracer_t(const char* config, const char* name)
  {
    cache = cache_sim_t::construct(config, name);
  }
  ~cache_memtracer_t()
  {
    delete cache;
  }
  void set_miss_handler(cache_sim_t* mh)
  {
    cache->set_miss_handler(mh);
  }
  void clean_invalidate(uint64_t addr, size_t bytes, bool clean, bool inval)
  {
    cache->clean_invalidate(addr, bytes, clean, inval);
  }
  void set_log(bool log)
  {
    cache->set_log(log);
  }
  void print_stats()
  {
    cache->print_stats();
  }

 protected:
  cache_sim_t* cache;
};

class icache_sim_t : public cache_memtracer_t
{
 public:
  icache_sim_t(const char* config, const char* name = "I$")
	  : cache_memtracer_t(config, name) {}
  bool interested_in_range(uint64_t UNUSED begin, uint64_t UNUSED end, access_type type)
  {
    return type == FETCH;
  }
  void trace(uint64_t addr, size_t bytes, access_type type)
  {
    if (type == FETCH) cache->access(addr, bytes, false);
  }
};

class dcache_sim_t : public cache_memtracer_t
{
 public:
  dcache_sim_t(const char* config, const char* name = "D$")
	  : cache_memtracer_t(config, name) {}
  bool interested_in_range(uint64_t UNUSED begin, uint64_t UNUSED end, access_type type)
  {
    return type == LOAD || type == STORE;
  }
  void trace(uint64_t addr, size_t bytes, access_type type)
  {
    if (type == LOAD || type == STORE) cache->access(addr, bytes, type == STORE);
  }
};

#endif
