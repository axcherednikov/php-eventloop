<?php

/**
 * IDE stub for ext-eventloop.
 *
 * This file is not executed — it provides autocompletion and type
 * information for IDEs like PhpStorm. Do not include or require it.
 */

namespace EventLoop;

enum CallbackType {
    case Defer;
    case Delay;
    case Repeat;
    case Readable;
    case Writable;
    case Signal;
}

final class InvalidCallbackError extends \Error {
    public readonly string $callbackId;

    public function __construct(string $callbackId, string $message) {}

    public function getCallbackId(): string {}
}

final class EventLoop {
    /**
     * Queue a microtask for immediate execution.
     *
     * @param \Closure(...):void $closure
     */
    public static function queue(\Closure $closure, mixed ...$args): void {}

    /**
     * Defer execution to the next iteration of the event loop.
     *
     * @param \Closure(string):void $closure Receives the callback identifier.
     * @return string A unique callback identifier.
     */
    public static function defer(\Closure $closure): string {}

    /**
     * Execute a callback after a delay in seconds.
     *
     * @param float $delay Delay in seconds.
     * @param \Closure(string):void $closure Receives the callback identifier.
     * @return string A unique callback identifier.
     */
    public static function delay(float $delay, \Closure $closure): string {}

    /**
     * Execute a callback repeatedly at an interval in seconds.
     *
     * @param float $interval Interval in seconds.
     * @param \Closure(string):void $closure Receives the callback identifier.
     * @return string A unique callback identifier.
     */
    public static function repeat(float $interval, \Closure $closure): string {}

    /**
     * Execute a callback when a stream becomes readable.
     *
     * @param resource $stream
     * @param \Closure(string, resource):void $closure Receives the callback identifier and stream.
     * @return string A unique callback identifier.
     */
    public static function onReadable(mixed $stream, \Closure $closure): string {}

    /**
     * Execute a callback when a stream becomes writable.
     *
     * @param resource $stream
     * @param \Closure(string, resource):void $closure Receives the callback identifier and stream.
     * @return string A unique callback identifier.
     */
    public static function onWritable(mixed $stream, \Closure $closure): string {}

    /**
     * Execute a callback when a signal is received.
     *
     * @param int $signal Signal number.
     * @param \Closure(string, int):void $closure Receives the callback identifier and signal number.
     * @return string A unique callback identifier.
     */
    public static function onSignal(int $signal, \Closure $closure): string {}

    /**
     * Enable a callback by its identifier.
     *
     * @throws InvalidCallbackError If the callback identifier is invalid.
     */
    public static function enable(string $callbackId): string {}

    /**
     * Disable a callback by its identifier.
     *
     * @throws InvalidCallbackError If the callback identifier is invalid.
     */
    public static function disable(string $callbackId): string {}

    /**
     * Cancel a callback by its identifier. No-op if the identifier is invalid.
     */
    public static function cancel(string $callbackId): void {}

    /**
     * Reference a callback so it keeps the event loop alive.
     *
     * @throws InvalidCallbackError If the callback identifier is invalid.
     */
    public static function reference(string $callbackId): string {}

    /**
     * Unreference a callback so it no longer keeps the event loop alive.
     *
     * @throws InvalidCallbackError If the callback identifier is invalid.
     */
    public static function unreference(string $callbackId): string {}

    /**
     * Check whether a callback is enabled.
     *
     * @throws InvalidCallbackError If the callback identifier is invalid.
     */
    public static function isEnabled(string $callbackId): bool {}

    /**
     * Check whether a callback is referenced.
     *
     * @throws InvalidCallbackError If the callback identifier is invalid.
     */
    public static function isReferenced(string $callbackId): bool {}

    /**
     * Get the type of a callback.
     *
     * @throws InvalidCallbackError If the callback identifier is invalid.
     */
    public static function getType(string $callbackId): CallbackType {}

    /**
     * Get all registered callback identifiers.
     *
     * @return list<string>
     */
    public static function getIdentifiers(): array {}

    /**
     * Run the event loop.
     *
     * @throws \Error If the event loop is already running.
     */
    public static function run(): void {}

    /**
     * Stop the event loop.
     */
    public static function stop(): void {}

    /**
     * Check if the event loop is running.
     */
    public static function isRunning(): bool {}

    /**
     * Set the error handler for callback exceptions.
     *
     * @param null|\Closure(\Throwable):void $errorHandler
     */
    public static function setErrorHandler(?\Closure $errorHandler): void {}

    /**
     * Get the current error handler.
     *
     * @return null|\Closure(\Throwable):void
     */
    public static function getErrorHandler(): ?\Closure {}

    /**
     * Get a Suspension object for the current execution context.
     */
    public static function getSuspension(): Suspension {}

    /**
     * Get the name of the active I/O driver.
     */
    public static function getDriver(): string {}
}

/**
 * @template T
 */
final class Suspension {
    /**
     * Suspend the current fiber and return to the event loop.
     *
     * @return T
     * @throws \Error If called outside of a fiber.
     * @throws \Throwable Rethrown from a {@see throw()} call.
     */
    public function suspend(): mixed {}

    /**
     * Resume the suspended fiber with a value.
     *
     * @param T $value
     * @throws \Error If the suspension is not suspended or resume was already called.
     */
    public function resume(mixed $value = null): void {}

    /**
     * Resume the suspended fiber by throwing an exception.
     *
     * @throws \Error If the suspension is not suspended or a resume/throw is already pending.
     */
    public function throw(\Throwable $throwable): void {}
}
