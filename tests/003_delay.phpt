--TEST--
EventLoop::delay() executes after specified delay
--EXTENSIONS--
eventloop
--FILE--
<?php

use EventLoop\EventLoop;

$start = hrtime(true);

EventLoop::delay(0.05, function () use ($start) {
    $elapsed = (hrtime(true) - $start) / 1e9;
    echo "Delay executed\n";
    // Allow some tolerance
    echo "Elapsed >= 0.04: " . ($elapsed >= 0.04 ? "yes" : "no") . "\n";
});

EventLoop::run();

echo "Done\n";

?>
--EXPECT--
Delay executed
Elapsed >= 0.04: yes
Done
