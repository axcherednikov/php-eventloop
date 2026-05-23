# Changelog

## 1.1.0 - 2026-05-23

### Fixed

- Routed microtask exceptions through the configured `EventLoop::setErrorHandler()` handler.
- Stopped microtask processing when the configured error handler throws.

### Changed

- Hardened the CI workflow permissions and avoided persisting checkout credentials.
- Optimized microtask queue processing to avoid repeated array shifting during dispatch.
- Added regression coverage for callback cancellation during timer dispatch and error handling.

## 1.0.2 - 2026-05-23

### Fixed

- Kept callbacks alive while they are being dispatched, preventing use-after-free when callbacks cancel themselves.
- Made I/O watcher dispatch resilient when watchers cancel themselves or each other during event processing.
- Restored previous signal handlers when signal watchers are cancelled or the request shuts down.
- Cancelled replaced signal watchers when registering a new watcher for the same signal.
- Rejected non-finite and excessively large timer values before scheduling timers.
- Guarded internal queue and timer heap capacity growth against integer overflow.

## 1.0.1 - 2026-05-23

### Changed

- Skipped unnecessary driver polling when no I/O watchers are active.

## 1.0.0 - 2026-05-23

### Added

- Initial native PHP event loop extension.
- Revolt-compatible `EventLoop\EventLoop` API for deferred callbacks, timers, repeaters, I/O watchers, signal watchers, error handlers, and loop control.
- Fiber suspension support via `EventLoop\Suspension`.
- Auto-selected I/O drivers for epoll, kqueue, poll, and select.
- PHP stubs and generated arginfo for the public API.
- `.phpt` test suite and benchmark scripts.
