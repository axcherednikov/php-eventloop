# ext-eventloop

A native PHP extension that brings a high-performance event loop directly into the engine. Inspired by and API-compatible with [Revolt](https://github.com/revoltphp/event-loop) -- not a replacement, but a **native alternative** written in C for zero-overhead async I/O.

> **Why?** Revolt is an excellent userland library. This extension takes the same proven API design and moves it into a PHP extension, eliminating userland dispatch overhead and leveraging OS-level I/O primitives (epoll, kqueue, poll) directly from C.

## Key Differences from Revolt

| | Revolt | ext-eventloop |
|---|---|---|
| Implementation | PHP userland | C extension |
| Installation | `composer require revolt/event-loop` | `phpize && make install` |
| I/O backend | Configurable (ev, event, uv) | Auto-detected (epoll / kqueue / poll / select) |
| Fiber suspension | Yes | Yes |
| API contract | `Revolt\EventLoop::*` | `EventLoop\EventLoop::*` |

The API surface mirrors Revolt's, so migrating between the two is straightforward -- adjust the namespace and you're done.

## Requirements

- PHP >= 8.1 (Fiber support required)
- A POSIX-compatible OS (Linux, macOS, FreeBSD, etc.)

## Installation

### Via PIE (recommended)

```bash
pie install axcherednikov/eventloop
```

### From source

```bash
git clone https://github.com/axcherednikov/php-eventloop.git
cd php-eventloop

phpize
./configure --enable-eventloop
make
make test
sudo make install
```

Then enable the extension:

```ini
; php.ini or conf.d/eventloop.ini
extension=eventloop
```

Verify:

```bash
php -m | grep eventloop
```

## Quick Start

```php
<?php

use EventLoop\EventLoop;

// Defer a callback to the next loop tick
EventLoop::defer(function (string $callbackId) {
    echo "Deferred callback executed\n";
});

// Delay execution by 1.5 seconds
EventLoop::delay(1.5, function (string $callbackId) {
    echo "This runs after 1.5 seconds\n";
});

// Repeat every 500ms
$id = EventLoop::repeat(0.5, function (string $callbackId) {
    echo "Tick\n";
});

// Cancel the repeater after 3 seconds
EventLoop::delay(3, function () use ($id) {
    EventLoop::cancel($id);
});

EventLoop::run();
```

## API Reference

All methods are static on `EventLoop\EventLoop`.

### Scheduling

| Method | Description |
|---|---|
| `queue(Closure $closure, mixed ...$args): void` | Queue a microtask for immediate execution |
| `defer(Closure $closure): string` | Defer to the next event loop iteration |
| `delay(float $delay, Closure $closure): string` | Execute after `$delay` seconds |
| `repeat(float $interval, Closure $closure): string` | Execute every `$interval` seconds |

### I/O Watchers

| Method | Description |
|---|---|
| `onReadable(resource $stream, Closure $closure): string` | Execute when a stream becomes readable |
| `onWritable(resource $stream, Closure $closure): string` | Execute when a stream becomes writable |

### Signal Handling

| Method | Description |
|---|---|
| `onSignal(int $signal, Closure $closure): string` | Execute when a signal is received |

### Callback Management

| Method | Description |
|---|---|
| `enable(string $id): string` | Enable a disabled callback |
| `disable(string $id): string` | Disable a callback (can be re-enabled) |
| `cancel(string $id): void` | Permanently cancel a callback |
| `reference(string $id): string` | Reference a callback (keeps the loop alive) |
| `unreference(string $id): string` | Unreference a callback |
| `isEnabled(string $id): bool` | Check if a callback is enabled |
| `isReferenced(string $id): bool` | Check if a callback is referenced |
| `getType(string $id): CallbackType` | Get the callback type |
| `getIdentifiers(): array` | Get all registered callback IDs |

### Loop Control

| Method | Description |
|---|---|
| `run(): void` | Run the event loop |
| `stop(): void` | Stop the event loop |
| `isRunning(): bool` | Check if the loop is running |
| `getDriver(): string` | Get the active I/O driver name |

### Error Handling

| Method | Description |
|---|---|
| `setErrorHandler(?Closure $handler): void` | Set the error handler for exceptions in callbacks |
| `getErrorHandler(): ?Closure` | Get the current error handler |

### Fiber Suspension

```php
$fiber = new Fiber(function () {
    $suspension = EventLoop::getSuspension();

    EventLoop::defer(function () use ($suspension) {
        $suspension->resume('hello');
    });

    $value = $suspension->suspend(); // "hello"
    echo $value; // "hello"
});

$fiber->start();
EventLoop::run();
```

| Method | Description |
|---|---|
| `Suspension::suspend(): mixed` | Suspend the current fiber |
| `Suspension::resume(mixed $value = null): void` | Resume with a value |
| `Suspension::throw(Throwable $e): void` | Resume by throwing an exception |

## I/O Drivers

The extension automatically selects the best I/O driver available on your system at compile time. There is no manual configuration needed -- you always get optimal performance for your platform.

| Driver | Platforms | Scalability | Notes |
|---|---|---|---|
| **epoll** | Linux 2.6+ | O(1) | Kernel tracks descriptors; returns only ready ones |
| **kqueue** | macOS, FreeBSD, OpenBSD | O(1) | Same principle as epoll, native to BSD systems |
| **poll** | Any POSIX | O(n) | No descriptor limit, but scans all on every call |
| **select** | Universal (fallback) | O(n) | Oldest API, limited to ~1024 descriptors |

**Selection priority:** epoll > kqueue > poll > select. The first one that compiles and initializes successfully wins.

In practice this means:
- **Linux servers** (the most common deployment) get **epoll** -- handles thousands of connections with near-zero overhead
- **macOS** (local development) gets **kqueue** -- equally efficient
- Older or exotic systems gracefully fall back to **poll** or **select**

Check which driver is active:

```php
echo EventLoop::getDriver(); // "epoll" on Linux, "kqueue" on macOS
```

## Benchmarks

**Environment:** PHP 8.5.4, Apple M1 Max, macOS, 100,000 iterations (average of 3 runs).
Revolt v1.0.8 tested with all four available drivers ([StreamSelect, Ev, Event, UV](https://revolt.run/extensions)). ext-eventloop using kqueue driver.

| Benchmark | Revolt StreamSelect | Revolt Ev | Revolt Event | Revolt UV | ext-eventloop |
|---|--:|--:|--:|--:|--:|
| `defer()` | 755,979 ops/sec | 726,468 ops/sec | 741,084 ops/sec | 730,488 ops/sec | **3,696,132 ops/sec** |
| `delay(0)` | 245,389 ops/sec | 480,866 ops/sec | 467,362 ops/sec | 185,068 ops/sec | **3,041,443 ops/sec** |
| `repeat(0)` | 694,763 ops/sec | 712,077 ops/sec | 74,929 ops/sec | 73,899 ops/sec | **17,971,855 ops/sec** |
| I/O register + cancel | 2,149,497 ops/sec | 2,056,740 ops/sec | 2,034,001 ops/sec | 2,000,214 ops/sec | **7,642,510 ops/sec** |
| Fiber suspend/resume | 219,349 ops/sec | 217,095 ops/sec | 220,924 ops/sec | 215,305 ops/sec | **242,691 ops/sec** |

> These benchmarks measure callback dispatch and scheduling throughput. The I/O backend (StreamSelect, Ev, Event, UV) primarily affects polling efficiency at high concurrency, not dispatch speed — that's why all four Revolt drivers show similar numbers here. ext-eventloop moves the entire dispatch path into C, which is where the difference comes from. Fiber performance is nearly identical because `suspend()`/`resume()` is handled by the Zend Engine directly.
>
> **[Run the benchmarks yourself →](benchmark/)**

## Migrating from Revolt

The API contract is intentionally compatible. In most cases, a namespace swap is all you need:

```diff
- use Revolt\EventLoop;
+ use EventLoop\EventLoop;
```

If you use `Revolt\EventLoop\Suspension`:

```diff
- use Revolt\EventLoop\Suspension;
+ use EventLoop\Suspension;
```

## Testing

```bash
make test
```

The extension ships with 26 `.phpt` tests covering defer, delay, repeat, I/O watchers, signals, suspensions, error handling, and edge cases.

## Acknowledgements

This project is built on the ideas and API design of [Revolt](https://github.com/revoltphp/event-loop) by Aaron Piotrowski, Niklas Keller, and contributors. Revolt's clean, well-thought-out API made it the natural foundation for a native implementation. Full credit to the Revolt team for defining the contract that this extension follows.

## License

Licensed under the [MIT License](LICENSE).