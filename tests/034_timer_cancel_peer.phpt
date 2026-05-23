--TEST--
Timer callbacks can cancel another due timer during dispatch
--EXTENSIONS--
eventloop
--FILE--
<?php

use EventLoop\EventLoop;
use EventLoop\InvalidCallbackError;

$first = null;
$second = EventLoop::delay(0.001, function () {
    echo "second\n";
});

$first = EventLoop::delay(0, function () use (&$second) {
    echo "first\n";
    EventLoop::cancel($second);
});

EventLoop::run();

try {
    EventLoop::isEnabled($second);
} catch (InvalidCallbackError) {
    echo "second cancelled\n";
}

?>
--EXPECT--
first
second cancelled
