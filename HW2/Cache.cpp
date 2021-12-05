#include <vector>
#include <math.h>

using std::vector;
using std::cout;
using std::endl;

#define ADDRESS_SIZE 32

typedef struct TagEntry
{
    unsigned tag;
    unsigned addr;
    bool dirty;
} TagEntry;


//Helper method to extract bits from given number
unsigned get_bits(unsigned number, int start, int N_bits){
	unsigned n = number; // n = ZZZZZ YYYY 00
	n = n >> start; //    	ZZZZZ YYYY 00 => ZZZZZ YYYY
	unsigned mask = 1 << N_bits; // mask = 00001 0000
	mask -= 1; // 0001000 => 0000111
	return n & mask;
}

class Cache
{
private:
    int size;
    int N_ways_bits;
    int block_size;
    bool is_write_allocate;
    vector< vector<TagEntry> > cache_table;

    //bits Calculations
    int N_blocks;
    int N_sets;
    int offset_bits;
    int set_bits;
    int tag_bits;
    int N_ways;
public:
    Cache(int size, int N_ways_bits, int block_size, unsigned access_time, bool is_write_allocate);
    bool seek(unsigned long int addr, char op, bool writeback);
    bool add_address(unsigned long int addr, unsigned long int* dirty_tag, bool* removed);
    void remove(unsigned long int addr);
    void mark_dirty(unsigned long int addr);
    double N_misses, N_hits;
    unsigned access_time;
};

Cache::Cache(int size, int N_ways_bits, int block_size, unsigned access_time, bool is_write_allocate) :
                size(size), N_ways_bits(N_ways_bits), block_size(block_size), access_time(access_time), is_write_allocate(is_write_allocate), N_misses(0), N_hits(0){
        
        this->N_ways = pow(2, this->N_ways_bits);

        this->N_blocks = pow(2, this->size) / pow(2, this->block_size);
        this->N_sets = N_blocks / this->N_ways;
        this->offset_bits = this->block_size;
        this->set_bits = log2(N_sets); //Can also be size - block_size - N_ways (in bits)
        this->tag_bits = ADDRESS_SIZE - set_bits - offset_bits;

        this->cache_table = vector< vector<TagEntry> >(N_sets);
}

bool Cache::seek(unsigned long int addr, char op, bool writeback){
    unsigned addr_set = get_bits(addr, this->offset_bits, this->set_bits);
    unsigned addr_tag = get_bits(addr, this->offset_bits + this->set_bits, this->tag_bits);

    // cout << "SET: " << addr_set << ", TAG: " << addr_tag << endl;

    for (vector<TagEntry>::iterator it = this->cache_table[addr_set].begin();
         it != this->cache_table[addr_set].end(); it++){
        if (it->tag == addr_tag){
            TagEntry new_entry;
            new_entry.tag = it->tag;
            new_entry.dirty = it->dirty;
            new_entry.addr = it->addr;
            
            this->cache_table[addr_set].erase(it);
            
            this->cache_table[addr_set].insert(this->cache_table[addr_set].begin(), new_entry);
            if (op == 'w')
                this->cache_table[addr_set].begin()->dirty = true;

            if (!writeback){
                this->N_hits++;
            }
            return true;
        }
    }
    // cout << "Misses: " << this->N_misses << "Hits: " << this->N_hits << endl;
    if (!writeback){
        this->N_misses++;
    }
    return false;
}

bool Cache::add_address(unsigned long int addr, unsigned long int* removed_addr, bool* removed){
    *removed = false;
    unsigned addr_set = get_bits(addr, this->offset_bits, this->set_bits);
    unsigned addr_tag = get_bits(addr, this->offset_bits + this->set_bits, this->tag_bits);

    TagEntry new_entry;
    new_entry.tag = addr_tag;
    new_entry.dirty = false;
    new_entry.addr = addr;

    this->cache_table[addr_set].insert(this->cache_table[addr_set].begin(), new_entry);
    if (this->cache_table[addr_set].size() > this->N_ways){
        *removed = true;
        TagEntry to_delete = this->cache_table[addr_set].back();
        this->cache_table[addr_set].pop_back();
        *removed_addr = to_delete.addr;
        if (to_delete.dirty)
            return true;
        return false;
    }
    return false;
}

void Cache::remove(unsigned long int addr) {
    unsigned addr_set = get_bits(addr, this->offset_bits, this->set_bits);
    unsigned addr_tag = get_bits(addr, this->offset_bits + this->set_bits, this->tag_bits);
    for (vector<TagEntry>::iterator it = this->cache_table[addr_set].begin();
         it != this->cache_table[addr_set].end(); it++){
        if (it->tag == addr_tag){
            this->cache_table[addr_set].erase(it);
            return;
        }
    }
}

void Cache::mark_dirty(unsigned long int addr) {
    unsigned addr_set = get_bits(addr, this->offset_bits, this->set_bits);
    unsigned addr_tag = get_bits(addr, this->offset_bits + this->set_bits, this->tag_bits);
    for (vector<TagEntry>::iterator it = this->cache_table[addr_set].begin();
         it != this->cache_table[addr_set].end(); it++){
        if (it->tag == addr_tag){
            it->dirty = true;
            return;
        }
    }
}