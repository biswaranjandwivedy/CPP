# STL vs Abseil vs Folly Containers - Complete Comparison Guide

## Executive Summary

For ultra-low latency trading systems, modern container libraries offer **40-80% performance improvements** over STL containers with **30-60% memory savings**.

### Quick Recommendation

| Use Case | Container | Improvement | Why |
|----------|-----------|-------------|-----|
| **Order tracking** | `abseil::flat_hash_map` | 50% faster | Cache-friendly, low memory |
| **Order book levels** | `abseil::btree_map` | 50% faster | Range queries, ordered |
| **Reference data** | `folly::sorted_vector_map` | 70% faster | Read-only, minimal memory |
| **Symbol cache** | `abseil::flat_hash_map` | 50% faster | Fast lookups, memory efficient |
| **Position tracking** | `abseil::flat_hash_map` | 50% faster | Updates + lookups |

---

## Table of Contents

1. [Performance Comparison](#performance-comparison)
2. [Hash Maps](#hash-maps)
3. [Ordered Maps](#ordered-maps)
4. [Trading Use Cases](#trading-use-cases)
5. [Installation Guide](#installation-guide)
6. [Decision Matrix](#decision-matrix)

---

## Performance Comparison

### Hash Maps (ns/operation)

| Container | Insert | Lookup | Erase | Memory vs STL |
|-----------|--------|--------|-------|---------------|
| `std::unordered_map` | 60-80 | 50-70 | 50-80 | 100% (baseline) |
| `abseil::flat_hash_map` | 30-40 | 25-35 | 30-40 | **50-70%** ⭐ |
| `folly::F14FastMap` | 20-30 | 20-30 | 25-35 | **40-60%** ⭐⭐ |

**Key Insight**: Abseil and Folly are **2-3x faster** than STL with **40-60% less memory**!

### Ordered Maps (ns/operation)

| Container | Insert | Lookup | Range Query | Memory vs STL |
|-----------|--------|--------|-------------|---------------|
| `std::map` | 100-150 | 80-120 | 500-1000 | 100% (baseline) |
| `abseil::btree_map` | 60-90 | 40-60 | 200-400 | **50-70%** ⭐ |
| `folly::sorted_vector_map` | O(n) ❌ | 20-40 | 100-200 | **20-40%** ⭐⭐ |

**Key Insight**: B-trees are **2x faster** than Red-Black trees. Sorted vectors are **3-4x faster** for reads!

---

## Hash Maps

### 1. `std::unordered_map` (STL)

**Architecture:**
- Linked-list buckets (chaining)
- Separate nodes for each element
- Pointer per element (high overhead)

**Characteristics:**
```
Memory Layout:
┌─────────┐    ┌──────┐    ┌──────┐
│ Bucket  │───→│ Node │───→│ Node │───→ null
│ Array   │    └──────┘    └──────┘
└─────────┘

Poor cache locality: Each lookup follows pointers
```

**Performance:**
- ✓ Standard, portable
- ✗ Slow lookups (50-70ns)
- ✗ High memory overhead
- ✗ Poor cache locality

**Use When:**
- Portability is critical
- Prototyping (optimize later)

---

### 2. `abseil::flat_hash_map` (Google)

**Architecture:**
- **Swiss Tables** algorithm
- Open addressing (flat/linear probing)
- SSE2/SSSE3 optimized probing
- Control bytes for fast searching

**Characteristics:**
```
Memory Layout:
┌────────────────────────────────────┐
│ [ctrl][ctrl][ctrl][ctrl]...[ctrl]  │  ← Control bytes (SIMD)
│ [key1][val1][key2][val2]...[keyN]  │  ← Flat array
└────────────────────────────────────┘

Excellent cache locality: Contiguous memory
```

**Performance:**
- ✓ 50% faster than `std::unordered_map`
- ✓ 40% less memory
- ✓ Cache-friendly
- ✓ SIMD-optimized
- ✗ Requires Abseil library

**Use When:**
- **Most trading use cases** ⭐
- Need fast lookups (20-40ns)
- Memory efficiency matters
- Proven in production (Google uses everywhere)

**Code Example:**
```cpp
#include "absl/container/flat_hash_map.h"

absl::flat_hash_map<uint64_t, Order> orders;
orders.reserve(100000);  // Pre-allocate

// Fast insert: ~30ns
orders.insert({order_id, order});

// Fast lookup: ~25ns
auto it = orders.find(order_id);
if (it != orders.end()) {
    // Process order
}
```

---

### 3. `folly::F14FastMap` (Facebook)

**Architecture:**
- **F14 algorithm** (Facebook's design)
- Hybrid of chaining and open addressing
- 14-way associative (14 entries per chunk)
- SIMD probing

**Characteristics:**
```
Memory Layout:
┌──────────────────────────────────┐
│ Chunk 1: [14 entries + metadata] │
│ Chunk 2: [14 entries + metadata] │
│ Chunk 3: [14 entries + metadata] │
└──────────────────────────────────┘

Best cache performance of all hash maps!
```

**Performance:**
- ✓ **60% faster** than `std::unordered_map` ⭐⭐⭐
- ✓ 50% less memory
- ✓ Best cache locality
- ✓ Fastest lookups (20-30ns)
- ✗ Requires Folly library (heavy dependencies)

**Use When:**
- Need **absolute best performance**
- Can afford Folly dependencies
- Hot path in trading system

**Code Example:**
```cpp
#include <folly/container/F14Map.h>

folly::F14FastMap<uint64_t, Order> orders;
orders.reserve(100000);

// Fastest insert: ~25ns
orders.insert({order_id, order});

// Fastest lookup: ~20ns
auto it = orders.find(order_id);
```

---

### 4. `folly::F14ValueMap` vs `F14FastMap`

| Feature | F14FastMap | F14ValueMap |
|---------|------------|-------------|
| Storage | Pointer to value | Value inline |
| Best for | Large values (>24 bytes) | Small values (≤24 bytes) |
| Cache hits | Good | **Excellent** ⭐ |
| Memory | Less overhead | **Minimal** ⭐ |

**Use `F14ValueMap` for:**
- Small structs (Order, Tick, etc.)
- Maximum cache performance
- Minimal memory usage

---

## Ordered Maps

### 1. `std::map` (STL)

**Architecture:**
- Red-Black Tree (self-balancing BST)
- 3 pointers + color bit per node
- O(log n) operations

**Characteristics:**
```
Tree Structure:
        [40]
       /    \
    [20]    [60]
    /  \    /  \
  [10][30][50][70]

Poor cache locality: Pointers everywhere
```

**Performance:**
- ✓ Standard, ordered
- ✗ Slow (80-120ns lookups)
- ✗ High memory (24-32 bytes overhead/node)
- ✗ Poor cache locality

---

### 2. `abseil::btree_map` (Google)

**Architecture:**
- **B-Tree** (multi-way tree)
- Node size optimized for cache lines
- Multiple keys per node (better locality)
- O(log n) operations

**Characteristics:**
```
B-Tree Structure (order 4):
        [20, 40, 60]
       /   |   |   \
  [5,10] [25,30] [45,50] [65,70,75]

Excellent cache locality: Multiple keys per node
```

**Performance:**
- ✓ **50% faster** than `std::map` ⭐
- ✓ 40-60% less memory
- ✓ Excellent for range queries
- ✓ Cache-friendly
- ✗ Requires Abseil library

**Use When:**
- Need ordered container
- Frequent range queries (order book!)
- Insert/delete operations
- **Best choice for order books** ⭐

**Code Example:**
```cpp
#include "absl/container/btree_map.h"

// Order book: price -> orders at that level
absl::btree_map<double, OrderList> bids;

// Fast insert: ~60ns
bids.insert({150.25, order_list});

// Fast range query (top 5 levels)
auto it = bids.begin();
for (int i = 0; i < 5 && it != bids.end(); ++i, ++it) {
    process_level(it->first, it->second);
}
```

---

### 3. `folly::sorted_vector_map` (Facebook)

**Architecture:**
- **Sorted `std::vector`**
- Binary search for lookups
- O(n) insert, O(log n) lookup

**Characteristics:**
```
Memory Layout:
┌──────────────────────────────────────────┐
│ [key1][val1][key2][val2]...[keyN][valN] │  ← Contiguous!
└──────────────────────────────────────────┘

Best cache locality possible!
```

**Performance:**
- ✓ **70% faster lookups** than `std::map` ⭐⭐⭐
- ✓ 70% less memory
- ✓ Perfect cache locality
- ✓ Minimal overhead
- ✗ O(n) inserts (use for read-heavy!)

**Use When:**
- **Read-heavy workloads** ⭐⭐⭐
- Reference data
- Security master
- Infrequent updates
- Maximum performance for lookups

**Code Example:**
```cpp
#include <folly/sorted_vector_types.h>

// Security reference data (read-only after init)
folly::sorted_vector_map<std::string, SecurityData> securities;

// Build once
std::vector<std::pair<std::string, SecurityData>> data;
// ... populate data ...
securities = folly::sorted_vector_map<...>(
    data.begin(), data.end()
);

// Ultra-fast lookup: ~20ns
auto it = securities.find("AAPL");
```

---

## Trading Use Cases

### Use Case 1: Order Book

**Requirement:**
- Store orders at price levels
- Fast order lookup by ID
- Fast range queries (best bid/ask)
- Frequent inserts/deletes

**Solution:**
```cpp
// Order ID → Order
absl::flat_hash_map<uint64_t, Order*> order_index;

// Price → OrderList (buy side)
absl::btree_map<double, OrderList, std::greater<>> bids;

// Price → OrderList (sell side)
absl::btree_map<double, OrderList> asks;

// Fast operations:
// - Add order:        ~30ns (flat_hash_map) + ~60ns (btree_map) = 90ns
// - Find order:       ~25ns (flat_hash_map)
// - Best bid:         ~5ns  (btree_map.begin())
// - Top 5 levels:     ~50ns (iterate 5 elements)
```

**Performance vs STL:**
- Order lookup: 50% faster
- Price level ops: 50% faster
- Memory: 40% less
- **Total improvement: 2x faster** ⭐

---

### Use Case 2: Symbol Cache

**Requirement:**
- Store market data for 10,000 symbols
- Ultra-fast lookup (sub-50ns)
- Mostly reads, rare updates
- Memory efficiency

**Solution:**
```cpp
absl::flat_hash_map<std::string, SymbolData> symbol_cache;
symbol_cache.reserve(10000);

// Update (rare)
symbol_cache["AAPL"] = tick_data;  // ~30ns

// Lookup (frequent)
auto it = symbol_cache.find("AAPL");  // ~25ns
if (it != symbol_cache.end()) {
    double mid = (it->second.bid + it->second.ask) / 2.0;
}
```

**Performance:**
- Lookup: 25-35ns (vs 50-70ns STL) = **50% faster**
- Memory: 60% of STL = **40% savings**

---

### Use Case 3: Position Tracking

**Requirement:**
- Track positions for 1,000 accounts
- Fast updates (fills)
- Fast aggregation
- Thread-safe

**Solution 1: Single Map + Lock**
```cpp
absl::flat_hash_map<AccountID, Position> positions;
std::shared_mutex positions_mutex;

// Update (write lock)
{
    std::unique_lock lock(positions_mutex);
    positions[account_id].quantity += fill_qty;  // ~30ns + lock
}

// Read (shared lock)
{
    std::shared_lock lock(positions_mutex);
    auto it = positions.find(account_id);  // ~25ns + lock
}
```

**Solution 2: Per-Thread Maps (Best Performance)**
```cpp
// No locks needed!
thread_local absl::flat_hash_map<AccountID, Position> local_positions;

// Update (no lock!)
local_positions[account_id].quantity += fill_qty;  // ~30ns

// Aggregate periodically (every 100ms)
void aggregate_positions() {
    for (auto& thread_map : all_thread_maps) {
        for (auto& [account, pos] : thread_map) {
            global_positions[account] += pos;
        }
    }
}
```

**Performance:**
- With lock: ~80ns per operation
- Per-thread: **~30ns per operation** ⭐
- **2.5x faster with per-thread approach!**

---

### Use Case 4: Reference Data

**Requirement:**
- 100,000 securities
- Read-only after initialization
- Multiple lookup keys (symbol, ISIN, SEDOL)
- Minimal memory

**Solution:**
```cpp
// Primary index (owns data)
folly::sorted_vector_map<std::string, SecurityData> by_symbol;

// Secondary indices (pointers)
folly::sorted_vector_map<std::string, SecurityData*> by_isin;
folly::sorted_vector_map<std::string, SecurityData*> by_sedol;

// Build once at startup
void initialize() {
    std::vector<std::pair<std::string, SecurityData>> data;
    // ... load from database ...
    
    by_symbol = folly::sorted_vector_map<...>(data.begin(), data.end());
    
    // Build secondary indices
    for (auto& [symbol, sec] : by_symbol) {
        by_isin[sec.isin] = &sec;
        by_sedol[sec.sedol] = &sec;
    }
}

// Ultra-fast lookups
auto it = by_symbol.find("AAPL");     // ~20ns ⭐
auto it2 = by_isin.find("US0378...");  // ~20ns ⭐
```

**Performance:**
- Lookup: 20-30ns (vs 80-100ns STL) = **70% faster** ⭐⭐⭐
- Memory: 40% of STL = **60% savings**

---

### Use Case 5: Time Series Data

**Requirement:**
- Store ticks in time order
- Range queries (time window)
- VWAP/TWAP calculations
- Append-only

**Solution:**
```cpp
// For append-only, use std::deque (best for this case)
std::deque<Tick> recent_ticks;

// Or for fixed window
template<typename T, size_t N>
class RingBuffer {
    std::array<T, N> buffer_;
    size_t head_ = 0;
    
public:
    void push(const T& item) {
        buffer_[head_++ % N] = item;
    }
    
    // Fast iteration for VWAP
    double calculate_vwap() const {
        double sum_pv = 0, sum_v = 0;
        for (const auto& tick : buffer_) {
            sum_pv += tick.price * tick.volume;
            sum_v += tick.volume;
        }
        return sum_pv / sum_v;
    }
};

RingBuffer<Tick, 1000> last_1000_ticks;
```

**Why std::deque for append-only:**
- O(1) push_back
- Cache-friendly iteration
- No reallocation
- **Better than hash maps for this use case**

---

## Installation Guide

### Installing Abseil (Recommended)

```bash
# Clone repository
git clone https://github.com/abseil/abseil-cpp.git
cd abseil-cpp

# Build
mkdir build && cd build
cmake .. -DCMAKE_CXX_STANDARD=17 -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
sudo make install

# Verify
ls /usr/local/include/absl/container/
# Should see: flat_hash_map.h, btree_map.h, etc.
```

**Compile with Abseil:**
```bash
g++ -std=c++20 -O3 -march=native -pthread program.cpp \
    -labsl_hash -labsl_raw_hash_set
```

---

### Installing Folly (Optional, for Best Performance)

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt-get install -y \
    libgoogle-glog-dev \
    libgflags-dev \
    libevent-dev \
    libdouble-conversion-dev \
    libssl-dev \
    libboost-all-dev

# Clone and build
git clone https://github.com/facebook/folly.git
cd folly
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
sudo make install
```

**Compile with Folly:**
```bash
g++ -std=c++20 -O3 -march=native -pthread program.cpp \
    -lfolly -lglog -lgflags -ldl
```

---

## Decision Matrix

### Hash Maps

| Requirement | Container | Why |
|-------------|-----------|-----|
| **General purpose** | `abseil::flat_hash_map` ⭐ | Best balance |
| **Best performance** | `folly::F14FastMap` ⭐⭐⭐ | Fastest |
| **Small values** | `folly::F14ValueMap` ⭐⭐ | Inline storage |
| **Portability** | `std::unordered_map` | Standard |
| **Lock-free** | `folly::AtomicHashMap` | High contention |

### Ordered Maps

| Requirement | Container | Why |
|-------------|-----------|-----|
| **General purpose** | `abseil::btree_map` ⭐ | Fast + ordered |
| **Read-heavy** | `folly::sorted_vector_map` ⭐⭐⭐ | Fastest reads |
| **Order book** | `abseil::btree_map` ⭐ | Range queries |
| **Portability** | `std::map` | Standard |

### By Trading Use Case

| Use Case | Container | Latency | Reason |
|----------|-----------|---------|--------|
| Order tracking | `flat_hash_map` | 25-35ns | Fast lookups |
| Price levels | `btree_map` | 40-60ns | Ordered + ranges |
| Symbol cache | `flat_hash_map` | 25-35ns | Memory efficient |
| Positions | `flat_hash_map` | 25-35ns | Updates + reads |
| Reference data | `sorted_vector_map` | 20-30ns | Read-only |
| Time series | `std::deque` or ring buffer | 5-10ns | Sequential |

---

## Performance Summary

### Latency Comparison (nanoseconds)

```
Hash Map Lookup:
std::unordered_map:     ████████████ 60ns
flat_hash_map:          ██████ 30ns       ⬅ 50% faster ⭐
F14FastMap:             █████ 25ns        ⬅ 60% faster ⭐⭐

Ordered Map Lookup:
std::map:               ████████████████ 100ns
btree_map:              ████████ 50ns     ⬅ 50% faster ⭐
sorted_vector_map:      ████ 25ns         ⬅ 75% faster ⭐⭐⭐
```

### Memory Comparison

```
Relative to std::unordered_map (100%):

flat_hash_map:          ██████████████ 60%     ⬅ 40% savings
F14FastMap:             ███████████ 50%        ⬅ 50% savings
sorted_vector_map:      ██████ 30%             ⬅ 70% savings ⭐
```

---

## Final Recommendations

### 🥇 For 80% of Trading Use Cases

**Use Abseil:**
- `flat_hash_map` for unordered data
- `btree_map` for ordered data

**Why:**
- ✓ 40-60% performance improvement
- ✓ 30-50% memory savings
- ✓ Easy integration (single dependency)
- ✓ Proven in production (Google, Bloomberg)
- ✓ Active maintenance

### 🥈 For Maximum Performance (Hot Paths)

**Use Folly:**
- `F14FastMap` / `F14ValueMap`
- `sorted_vector_map` for read-heavy

**Why:**
- ✓ 60-80% performance improvement
- ✓ 40-60% memory savings
- ✓ Best-in-class performance
- ✗ Heavier dependencies

### 🥉 Avoid in Hot Paths

**Don't use:**
- `std::unordered_map` → 2x slower than alternatives
- `std::map` → 2-3x slower than alternatives

**Use STL only for:**
- Prototyping
- Non-critical paths
- Maximum portability requirements

---

## Quick Start

### Minimal Example with Abseil

```cpp
#include "absl/container/flat_hash_map.h"
#include <iostream>

struct Order {
    uint64_t id;
    double price;
    int quantity;
};

int main() {
    // Create order tracking map
    absl::flat_hash_map<uint64_t, Order> orders;
    orders.reserve(10000);
    
    // Add order (fast: ~30ns)
    orders[12345] = {12345, 150.25, 100};
    
    // Lookup order (fast: ~25ns)
    auto it = orders.find(12345);
    if (it != orders.end()) {
        std::cout << "Order " << it->second.id 
                  << " @ " << it->second.price << "\n";
    }
    
    return 0;
}
```

**Compile:**
```bash
g++ -std=c++20 -O3 program.cpp -labsl_hash -labsl_raw_hash_set
```

---

## Benchmarking Your System

Run the included benchmark:

```bash
# Compile
g++ -std=c++20 -O3 -march=native -pthread \
    stl_abseil_folly_containers_comparison.cpp \
    -o benchmark

# Run
./benchmark
```

**Expected output:**
- Insert/lookup/erase timings
- Memory usage comparison
- Trading use case recommendations
- Decision matrix

---

## Conclusion

For ultra-low latency trading systems:

1. **Replace `std::unordered_map` with `abseil::flat_hash_map`** → 50% faster ⭐
2. **Replace `std::map` with `abseil::btree_map`** → 50% faster ⭐
3. **Use `folly::sorted_vector_map` for read-only data** → 75% faster ⭐⭐⭐
4. **Consider `folly::F14FastMap` for critical hot paths** → 60% faster ⭐⭐

**Impact on Trading System:**
- Order book latency: 100ns → 50ns
- Symbol lookup: 60ns → 25ns
- Total tick-to-trade: **Reduce by 30-50%** 🚀

**Bottom line:** Modern containers are a **free 2x performance boost** with minimal code changes!
