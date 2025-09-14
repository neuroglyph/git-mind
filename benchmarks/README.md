# 🦬 git-mind Benchmarks

## The Wildebeest Stampede Benchmark

> "Long live the king!" - Scar, moments before disaster

### What is this

Just as Disney invented AI flocking algorithms for The Lion King's wildebeest stampede scene, we use Roaring Bitmaps to handle massive edge queries in git-mind.

This benchmark simulates Mufasa's worst day:

- Thousands of wildebeest (edges) stampeding
- Half running TO Mufasa, half running FROM
- Can our cache help him survive? (Spoiler: Yes!)

### Running the Stampede

```bash
cd benchmarks
make run
```

### Expected Results

```
🦬🦬🦬 WILDEBEEST STAMPEDE BENCHMARK 🦬🦬🦬

🦬 STARTING STAMPEDE with 1000 wildebeest!
⚡ MUFASA WITHOUT CACHE: ~50ms
🦁 SIMBA WITH ROARING CACHE: ~2ms
Speedup: 25x

🦬 STARTING STAMPEDE with 10000 wildebeest!  
⚡ MUFASA WITHOUT CACHE: ~500ms
🦁 SIMBA WITH ROARING CACHE: ~5ms
Speedup: 100x

🦬 STARTING STAMPEDE with 100000 wildebeest!
⚡ MUFASA WITHOUT CACHE: ~5000ms
🦁 SIMBA WITH ROARING CACHE: ~10ms
Speedup: 500x
```

### The Moral

With O(N) journal scans, Mufasa doesn't stand a chance against the stampede. But with O(log N) Roaring Bitmap queries, Simba saves the day!

No TODOs were harmed in the making of this benchmark.

---

_"Remember who you are..." - A developer with a working cache!_ 🦁
