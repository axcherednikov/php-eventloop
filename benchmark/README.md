# Benchmarks

Callback dispatch throughput benchmarks comparing ext-eventloop with Revolt across all available drivers.

## Setup

```bash
cd benchmark
composer install
```

For Revolt driver comparison, install the desired extensions:

```bash
pecl install ev        # EvDriver
pecl install event     # EventDriver (requires libevent)
pecl install uv-beta   # UvDriver (requires libuv)
```

## Running

```bash
# ext-eventloop (requires ext-eventloop installed)
php bench_eventloop.php [iterations]

# Revolt drivers
php bench_revolt_stream_select.php [iterations]
php bench_revolt_ev.php [iterations]
php bench_revolt_event.php [iterations]
php bench_revolt_uv.php [iterations]
```

Default: 100,000 iterations.

## What is measured

| Benchmark | What it tests |
|---|---|
| `defer()` | Queue and dispatch deferred callbacks |
| `delay(0)` | Schedule and fire zero-delay timers |
| `repeat(0)` | Repeated timer dispatch throughput |
| I/O register + cancel | onReadable registration and cancellation overhead |
| Fiber suspend/resume | Fiber context switching via Suspension |

These benchmarks measure **callback dispatch overhead** — how fast the loop can register, schedule, and invoke callbacks. The I/O backend primarily affects polling efficiency at high concurrency, not dispatch speed.

## File structure

| File | Description |
|---|---|
| `bench.php` | Shared benchmark logic |
| `bench_eventloop.php` | ext-eventloop entry point |
| `bench_revolt_stream_select.php` | Revolt StreamSelectDriver |
| `bench_revolt_ev.php` | Revolt EvDriver (pecl/ev) |
| `bench_revolt_event.php` | Revolt EventDriver (pecl/event) |
| `bench_revolt_uv.php` | Revolt UvDriver (pecl/uv) |
