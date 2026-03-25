--TEST--
EventLoop::disable() and enable() control callback execution
--EXTENSIONS--
eventloop
--FILE--
<?php

use EventLoop\EventLoop;

$count = 0;

$id = EventLoop::repeat(0.01, function (string $callbackId) use (&$count) {
    $count++;
    echo "Tick $count\n";
    if ($count >= 2) {
        EventLoop::cancel($callbackId);
    }
});

// Disable then re-enable
EventLoop::disable($id);
echo "isEnabled after disable: " . var_export(EventLoop::isEnabled($id), true) . "\n";

EventLoop::enable($id);
echo "isEnabled after enable: " . var_export(EventLoop::isEnabled($id), true) . "\n";

EventLoop::run();

echo "Total: $count\n";

?>
--EXPECT--
isEnabled after disable: false
isEnabled after enable: true
Tick 1
Tick 2
Total: 2
