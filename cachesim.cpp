#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <math.h>
#include <vector>
#include <algorithm>

// https://occs.oberlin.edu/~ctaylor/classes/210SP13/cache.html

// Default Values
int associativity = 1;    // Associativity of cache
int blocksize_bytes = 32; // Cache Block size in bytes
int cachesize_kb = 64;    // Cache size in KB
int miss_penalty = 30;
int dirty_write_penalty = 2;

class CacheSimulator
{
public:
  // Explicity Parameters
  int associativity;  // Associativity of cache
  int blocksize_byte; // Cache Block size in bytes
  int cachesize_kb;   // Cache size in KB
  int miss_penalty;
  int dirty_write_penalty;

  // Implicit Parameters
  int num_blocks;
  int num_sets;

  // Addressing Parameters
  // Address are portioned into the following in a set-associative or direct-mapped cache
  // TAG | INDEX | BLOCK_OFFSET
  int num_offset_bits;
  int num_index_bits;
  int tag_mask;

  // Represent cache structure as vectors of the metadata and data
  std::vector<int> tags;
  std::vector<int> valid;
  std::vector<int> priority;
  std::vector<int> dirty;

  // Statistics
  int stat_instructions = 0;
  int stat_cycles = 0;
  int stat_memory_accesses = 0;
  int stat_dirty_evictions = 0;
  int stat_load_misses = 0;
  int stat_store_misses = 0;
  int stat_load_hits = 0;
  int stat_store_hits = 0;

  CacheSimulator(int a, int bs, int cs, int mp, int dwp)
  {
    associativity = a;
    blocksize_byte = bs;
    cachesize_kb = cs;
    miss_penalty = mp;
    dirty_write_penalty = dwp;

    // Calculate Implicit/Addressing Parameters
    num_blocks = (1024 * cachesize_kb) / blocksize_bytes;
    num_sets = num_blocks / associativity;
    num_offset_bits = std::log2(blocksize_bytes);
    num_index_bits = std::log2(num_sets);
    tag_mask = num_index_bits + num_offset_bits;

    tags.resize(num_blocks, 0);
    valid.resize(num_blocks, 0);
    priority.resize(num_blocks, 0);
    dirty.resize(num_blocks, 0);
  }

  // Load and Store instructions perform almost the same subtasks
  std::tuple<bool, bool> instruction(int type, unsigned int address)
  {
    int index = get_index_bits(address);
    int tag = get_tag_bits(address);

    int invalid_index = -1;
    int hit_index = -1;
    bool hit = false;

    // Iterate through sub-vector containing all the blocks in a set
    for (int i = index; i < index + associativity; i++)
    {
      if (!valid[i])
      {
        invalid_index = i;
        continue;
      }

      // Hit Miss
      if (tags[i] != tag)
      {
        continue;
      }

      // Hit Occured
      hit = true;
      hit_index = i;

      // If instruction type is store then set dirty bit to 1
      dirty[hit_index] |= type;
      break;
    }

    // Cache Miss Occured
    // Replace an invalid block in the set or
    // Perform Least Recently Used (LRU) replacement scheme
    int new_index = hit_index;
    bool dirty_writeback = false;
    if (!hit)
    {
      if (invalid_index >= 0)
      {
        valid[invalid_index] = 1;
        new_index = invalid_index;
      }
      else
      {
        // Determine index with least priority i.e. highest value
        new_index = max_element_index(priority, index, associativity);
        // If the block selected for eviction is dirty (i.e., it has been modified), a dirty writeback occurs.
        dirty_writeback = dirty[new_index];
      }
      dirty[new_index] = type;
      tags[new_index] = tag;
    }

    // Update Priority
    for (int i = index; i < index + associativity; i++)
    {
      if (priority[index] < priority[new_index])
      {
        priority[index] += 1;
      }
    }
    priority[new_index] = 1;

    return {hit, dirty_writeback};
  };

  void update_statistics(int itype, int icount, bool hit, bool dirty_writeback)
  {
    stat_cycles += icount;
    stat_instructions += icount;
    stat_memory_accesses++;

    if (hit)
    {
      if (itype)
      {
        stat_store_hits++;
      }
      else
      {
        stat_load_hits++;
      }
    }
    else
    {
      if (itype)
      {
        stat_store_misses++;
      }
      else
      {
        stat_load_misses++;
      }
      stat_cycles += miss_penalty;
    }

    if (dirty_writeback)
    {
      stat_cycles += dirty_write_penalty;
      stat_dirty_evictions++;
    }
  }

  void display_statistics()
  {
    int misses = stat_load_misses + stat_store_misses;
    double overall_miss_rate = (double)misses / double(stat_memory_accesses);
    double read_miss_rate = (double)stat_load_misses / double(stat_memory_accesses - stat_store_hits - stat_store_misses);
    double total_cpi = (double)stat_cycles / (double)stat_instructions;

    printf("Simulation results:\n");
    printf("\texecution time %d cycles\n", stat_cycles);
    printf("\tinstructions %d\n", stat_instructions);
    printf("\tmemory accesses %d\n", stat_memory_accesses);
    printf("\toverall miss rate %.2f\n", overall_miss_rate);
    printf("\tread miss rate %.2f\n", read_miss_rate);
    // printf("\tmemory CPI %.2f\n", memory_cpi);
    printf("\ttotal CPI %.2f\n", total_cpi);
    // printf("\taverage memory access time %.2f cycles\n",  ?);
    printf("dirty evictions %d\n", stat_dirty_evictions);
    printf("load_misses %d\n", stat_load_misses);
    printf("store_misses %d\n", stat_store_misses);
    printf("load_hits %d\n", stat_load_hits);
    printf("store_hits %d\n", stat_store_hits);
  }

private:
  int max_element_index(const std::vector<int> &vec, int start_index, int offset)
  {
    // Check if the start_index and offset are valid
    if (start_index < 0 || start_index >= vec.size() || offset <= 0)
    {
      return -1; // Invalid input, return -1
    }

    // Calculate the end index for the slice
    int end_index = start_index + offset;

    // Use std::max_element to find the iterator to the largest element in the slice
    auto it = std::max_element(vec.begin() + start_index, vec.begin() + end_index);

    // Calculate the index of the maximum element relative to the entire vector
    int index = std::distance(vec.begin(), it);

    return index;
  }

  int get_index_bits(unsigned int address)
  {
    return (address >> num_offset_bits) & (num_blocks - 1);
  }

  int get_tag_bits(unsigned int address)
  {
    return address >> tag_mask;
  }
};

void print_usage()
{
  printf("Usage: gunzip2 -c <tracefile> | ./cache -a assoc -l blksz -s size -mp mispen\n");
  printf("  tracefile : The memory trace file\n");
  printf("  -a assoc : The associativity of the cache\n");
  printf("  -l blksz : The blocksize (in bytes) of the cache\n");
  printf("  -s size : The size (in KB) of the cache\n");
  printf("  -mp mispen: The miss penalty (in cycles) of a miss\n");
  exit(0);
}

int main(int argc, char *argv[])
{

  long address;
  int loadstore, icount;
  char marker;

  int i = 0;
  int j = 1;
  // Process the command line arguments
  // Process the command line arguments
  while (j < argc)
  {
    if (strcmp("-a", argv[j]) == 0)
    {
      j++;
      if (j >= argc)
        print_usage();
      associativity = atoi(argv[j]);
      j++;
    }
    else if (strcmp("-l", argv[j]) == 0)
    {
      j++;
      if (j >= argc)
        print_usage();
      blocksize_bytes = atoi(argv[j]);
      j++;
    }
    else if (strcmp("-s", argv[j]) == 0)
    {
      j++;
      if (j >= argc)
        print_usage();
      cachesize_kb = atoi(argv[j]);
      j++;
    }
    else if (strcmp("-mp", argv[j]) == 0)
    {
      j++;
      if (j >= argc)
        print_usage();
      miss_penalty = atoi(argv[j]);
      j++;
    }
    else
    {
      print_usage();
    }
  }

  // print out cache configuration
  printf("Cache parameters:\n");
  printf("\tCache Size (KB)\t\t\t%d\n", cachesize_kb);
  printf("\tCache Associativity\t\t%d\n", associativity);
  printf("\tCache Block Size (bytes)\t%d\n", blocksize_bytes);
  printf("\tMiss penalty (cyc)\t\t%d\n", miss_penalty);
  printf("\n");

  CacheSimulator cache(associativity, blocksize_bytes, cachesize_kb, miss_penalty, dirty_write_penalty);

  while (scanf("%c %d %lx %d\n", &marker, &loadstore, &address, &icount) != EOF)
  {
    // printf("\t%c %d %lx %d\n", marker, loadstore, address, icount );

    auto [hit, dirty_writeback] = cache.instruction(loadstore, address);
    cache.update_statistics(loadstore, icount, hit, dirty_writeback);
  }

  cache.display_statistics();
}
