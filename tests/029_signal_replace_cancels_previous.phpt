--TEST--
EventLoop::onSignal() replaces and cancels an existing signal watcher
--EXTENSIONS--
eventloop
pcntl
posix
--FILE--
<?php

use EventLoop\EventLoop;
use EventLoop\InvalidCallbackError;

$first = EventLoop::onSignal(SIGUSR1, function () {
    echo "first\n";
});

$second = EventLoop::onSignal(SIGUSR1, function (string $callbackId) {
    echo "second\n";
    EventLoop::cancel($callbackId);
    EventLoop::stop();
});

try {
    EventLoop::isEnabled($first);
} catch (InvalidCallbackError) {
    echo "first cancelled\n";
}

posix_kill(posix_getpid(), SIGUSR1);
EventLoop::run();

echo "done\n";

?>
--EXPECT--
first cancelled
second
done
