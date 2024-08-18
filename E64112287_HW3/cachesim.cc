// See LICENSE for license details.

#include "cachesim.h"
#include "common.h"
#include <cstdlib>
#include <iostream>
#include <iomanip>

/*
 * 需要自行modify(較多細節)的是:
 * check_tag, access(maybe?), victimize
 */

// contructor, 根據給定的規格去初始化虛擬的cache instance
cache_sim_t::cache_sim_t(size_t _sets, size_t _ways, size_t _linesz, const char* _name)
: sets(_sets), ways(_ways), linesz(_linesz), name(_name), log(false)
{
  init();
}

// 在init()裡率先檢查規格，如果規格有問題就會出現error提示然後結束模擬
static void help()
{
  std::cerr << "Cache configurations must be of the form" << std::endl;
  std::cerr << "  sets:ways:blocksize" << std::endl;
  std::cerr << "where sets, ways, and blocksize are positive integers, with" << std::endl;
  std::cerr << "sets and blocksize both powers of two and blocksize at least 8." << std::endl;
  exit(1);
}

// produce fully or n-way set associative cache instance
cache_sim_t* cache_sim_t::construct(const char* config, const char* name)
{
  // config = "sets:ways:block size"
  // spike --isa=RV64GC --dc=8:4:32 $RISCV/riscv64-unknown-elf/bin/pk <your program>
  // HW3's config = 8:4:32

  // 檢查給的規格參數是否格式錯誤，錯誤就輸出help msg然後結束
  const char* wp = strchr(config, ':');
  if (!wp++) help();
  const char* bp = strchr(wp, ':');
  if (!bp++) help();

  size_t sets = atoi(std::string(config, wp).c_str());
  size_t ways = atoi(std::string(wp, bp).c_str());
  size_t linesz = atoi(bp);

  // 因為HW3給的是8:4:32, 即sets=8, ways=4, block size = 32 bytes
  // 則sets=8, sets!=1所以執行的是 n-way set associative cache contructor
  // 所以HW3先不改fully associative cache的class
  if (ways > 4 /* empirical */ && sets == 1)
    return new fa_cache_sim_t(ways, linesz, name);
  return new cache_sim_t(sets, ways, linesz, name);
}

void cache_sim_t::init()
{
  /* 
   * check if sets and linesz are power of 2 or not, 
   * the form of power of 2: 0...010...0
   * they must obey the configuration which can be find in help()
   */
  if (sets == 0 || (sets & (sets-1)))
    help();
  if (linesz < 8 || (linesz & (linesz-1)))
    help();

  /*
   * to determine the number of bits of right shifting an address
   * then we can get index field to the set we want
   * idx_shift=log2(linesz)
   */
  idx_shift = 0;
  for (size_t x = linesz; x>1; x >>= 1)
    idx_shift++;

  tags = new uint64_t[sets*ways]();// generate tag value array
  /*
   * 一個cache可以放sets*ways個blocks
   * 每個不同位置block會去占用格子，它們會放在不同set或不同way
   * 就表示需要有sets*ways個空間放置tag values
   */

  read_accesses = 0;
  read_misses = 0;
  bytes_read = 0;
  write_accesses = 0;
  write_misses = 0;
  bytes_written = 0;
  writebacks = 0;

  // initialize the fifo_queues
  fifo_queues = std::vector<std::deque<uint64_t>>(sets, std::deque<uint64_t>(ways, 0));

  miss_handler = NULL;
}

// give an cache instance to generate a new cache instance with the same configuration
cache_sim_t::cache_sim_t(const cache_sim_t& rhs)
 : sets(rhs.sets), ways(rhs.ways), linesz(rhs.linesz),
   idx_shift(rhs.idx_shift), name(rhs.name), log(false), fifo_queues(rhs.fifo_queues)
{
  tags = new uint64_t[sets*ways];
  memcpy(tags, rhs.tags, sets*ways*sizeof(uint64_t));
}

// decontructor
cache_sim_t::~cache_sim_t()
{
  print_stats();
  delete [] tags;
  // 不需要特別去delete fifo_queues，STL的容器會自動管理
}

// print status of the cache
void cache_sim_t::print_stats()
{
  float mr = 100.0f*(read_misses+write_misses)/(read_accesses+write_accesses);

  std::cout << std::setprecision(3) << std::fixed;
  std::cout << name << " ";
  std::cout << "Bytes Read:            " << bytes_read << std::endl;
  std::cout << name << " ";
  std::cout << "Bytes Written:         " << bytes_written << std::endl;
  std::cout << name << " ";
  std::cout << "Read Accesses:         " << read_accesses << std::endl;
  std::cout << name << " ";
  std::cout << "Write Accesses:        " << write_accesses << std::endl;
  std::cout << name << " ";
  std::cout << "Read Misses:           " << read_misses << std::endl;
  std::cout << name << " ";
  std::cout << "Write Misses:          " << write_misses << std::endl;
  std::cout << name << " ";
  std::cout << "Writebacks:            " << writebacks << std::endl;
  std::cout << name << " ";
  std::cout << "Miss Rate:             " << mr << '%' << std::endl;
}

/*description to be added*/
uint64_t* cache_sim_t::check_tag(uint64_t addr)
{
  /*
   * 萃取出idx值:
   * right shift以去除offset field
   * AND將tag field變成00...00
   */
  size_t idx = (addr >> idx_shift) & (sets-1);

  /* 
   * 窩不確定
   * 消除offset field且讓此次的可能即將存入block變成可用的
   */
  size_t tag = (addr >> idx_shift) | VALID;// 目標值, 將valid bit設為1

  // query the queue
  std::deque<uint64_t>& dq = fifo_queues[idx];// 取得reference，直接使用vector中的正本

  for(uint64_t& x : dq) {
    if(tag == (x & ~DIRTY)) {
      return &x;
    }
  }

  return NULL;
  

  /*
   * 檢查指定的set中是否有我要的tag:
   * 迭代尋找指定的set中的所有標籤是否含有所求的tag value
   * |v|d|0...0|tag|idx|, |0...0|有idx_shift個0
   * 我們從addr算出來的tag的dirty為0，但取出來的tag未必dirty為0
   * 所以需要無視dirty bit去尋找tag value
   * 所以是~DIRTY這樣子取出來的tag value的dirty bit就會被AND出0然後其他bit和1做AND則濾出原始值
   * 同時也可以檢查取出的tag value是否valid
   */

  /*
  for (size_t i = 0; i < ways; i++)
    if (tag == (tags[idx*ways + i] & ~DIRTY))
      return &tags[idx*ways + i];

  return NULL;
  */
}

//TODO
/*
 * when miss, you will choose a block(victim) to be replace
 * if the set is full, then pop the head
 * if the set is not full, 
 */
uint64_t cache_sim_t::victimize(uint64_t addr)
{
  /*
   * |tag|idx|
   * It is impossible to have the same tag value in the same set
   * if |tag|idx|_1 == |tag|idx|_2, then they are the same block 
   */
  size_t idx = (addr >> idx_shift) & (sets-1);//查詢指定的set -> sets[idx]

  std::deque<uint64_t>& dq = fifo_queues[idx];
  
  /*
   * no matter the set is full or not, pop the head and push back the new tag into the deque. 
   * but actually the deque is full of 0s when we first select a victim
   * it contains: 
   * Set 0: 0 0 0 0 
   * Set 1: 0 0 0 0 
   * Set 2: 0 0 0 0 
   * Set 3: 0 0 0 0 
   * Set 4: 0 0 0 0 
   * Set 5: 0 0 0 0 
   * Set 6: 0 0 0 0 
   * Set 7: 0 0 0 0 
   * because we inialize it in init()
   * we must pop and push when we call victimize(), so as to keep every set of the cache has #ways space to be use
   * or the size of a set will getting bigger
   */
  uint64_t victim = dq.front();
  dq.pop_front();
  dq.push_back((addr >> idx_shift) | VALID);
  return victim;

  /* LFSR replacement policy 
  size_t way = lfsr.next() % ways;// select a radom number which is in 0 ~ (ways-1)
  uint64_t victim = tags[idx*ways + way];// get the selected tag value
  tags[idx*ways + way] = (addr >> idx_shift) | VALID;//clear the tag of the way 
  return victim;// return selected tag value
  */
}

/* 
 * access the cache(read or write)
 * 根據給定的address(main memory中的地址即addr)
 * addr具有idx和tag
 * 透過idx value可以找到指定的set in the cache
 * 然後在指定的set裡面尋找看看有沒有我要的tag value
 * 有找到就執行read or write
 * read:讀取數據，不會進行修改，也就是不會有寫回memory的議題只會取代cache裡的block
 * write:寫入數據，更新block數值
 * 需要更新block的話(cache中指定的set沒有所求的tag值)
 * 就是根據policy剔除某個tag，然後把新的tag value放入data member中(fifo_queue或tags array)
 */
void cache_sim_t::access(uint64_t addr, size_t bytes, bool store)
{
  /*
   * store == true -> write
   * store == false -> read
   * 計算讀寫access次數與讀寫所操作的byte數
   */
  store ? write_accesses++ : read_accesses++;
  (store ? bytes_written : bytes_read) += bytes;

  // check hit or miss
  uint64_t* hit_way = check_tag(addr);//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

  // when hit
  if (likely(hit_way != NULL))
  {
    // hit and write, then setup dirty bit
    if (store)
      *hit_way |= DIRTY;
    return;
  }

  /*----------------我是hit/miss處理分隔線-------------------*/

  // when miss
  store ? write_misses++ : read_misses++;
  if (log)
  {
    std::cerr << name << " "
              << (store ? "write" : "read") << " miss 0x"
              << std::hex << addr << std::endl;
  }

  // get the selected tag value
  uint64_t victim = victimize(addr);//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

  // 確認victim是否可用且modified，都是的話就往下一等級儲存裝置寫回去
  if ((victim & (VALID | DIRTY)) == (VALID | DIRTY))
  {
    uint64_t dirty_addr = (victim & ~(VALID | DIRTY)) << idx_shift;
    if (miss_handler)
      miss_handler->access(dirty_addr, linesz, true);
    writebacks++;
  }

  // 取得想要但是miss的data
  if (miss_handler)
    miss_handler->access(addr & ~(linesz-1), linesz, false);

  //
  if (store)
    *check_tag(addr) |= DIRTY;
  
}

// clean or invalidate cache, I don't want to read
void cache_sim_t::clean_invalidate(uint64_t addr, size_t bytes, bool clean, bool inval)
{
  uint64_t start_addr = addr & ~(linesz-1);
  uint64_t end_addr = (addr + bytes + linesz-1) & ~(linesz-1);
  uint64_t cur_addr = start_addr;
  while (cur_addr < end_addr) {
    uint64_t* hit_way = check_tag(cur_addr);
    if (likely(hit_way != NULL))
    {
      if (clean) {
        if (*hit_way & DIRTY) {
          writebacks++;
          *hit_way &= ~DIRTY;
        }
      }

      if (inval)
        *hit_way &= ~VALID;
    }
    cur_addr += linesz;
  }
  if (miss_handler)
    miss_handler->clean_invalidate(addr, bytes, clean, inval);
}

// fully的暫時不動因為HW3用不到
fa_cache_sim_t::fa_cache_sim_t(size_t ways, size_t linesz, const char* name)
  : cache_sim_t(1, ways, linesz, name)
{
}

uint64_t* fa_cache_sim_t::check_tag(uint64_t addr)
{
  auto it = tags.find(addr >> idx_shift);
  return it == tags.end() ? NULL : &it->second;
}

uint64_t fa_cache_sim_t::victimize(uint64_t addr)
{
  uint64_t old_tag = 0;
  if (tags.size() == ways)
  {
    auto it = tags.begin();
    std::advance(it, lfsr.next() % ways);
    old_tag = it->second;
    tags.erase(it);
  }
  tags[addr >> idx_shift] = (addr >> idx_shift) | VALID;
  return old_tag;
}
