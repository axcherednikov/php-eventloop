--TEST--
EventLoop::stop() stops the event loop
--EXTENSIONS--
eventloop
--FILE--
<?php

use EventLoop\EventLoop;

$count = 0;

EventLoop::repeat(0.01, function () use (&$count) {
    $count++;
    echo "Tick $count\n";
    if ($count >= 2) {
        EventLoop::stop();
    }
});

echo "isRunning before: " . var_export(EventLoop::isRunning(), true) . "\n";

EventLoop::run();

echo "isRunning after: " . var_export(EventLoop::isRunning(), true) . "\n";
echo "Stopped after $count ticks\n";

?>
--EXPECT--
isRunning before: false
Tick 1
Tick 2
isRunning after: false
Stopped after 2 ticks
