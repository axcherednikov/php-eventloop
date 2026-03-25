--TEST--
Suspension basic suspend and resume
--EXTENSIONS--
eventloop
--FILE--
<?php

use EventLoop\EventLoop;
use EventLoop\Suspension;

$fiber = new Fiber(function () {
    $suspension = EventLoop::getSuspension();

    echo "Before suspend\n";
    $value = $suspension->suspend();
    echo "After suspend, got: $value\n";
});

$fiber->start();

echo "Fiber suspended\n";

// Resume the fiber by getting its suspension and resuming
// In practice, this would be done from within an event callback
$suspension = EventLoop::getSuspension();

// For this test, use Fiber directly
$fiber->resume("hello");

echo "Done\n";

?>
--EXPECT--
Before suspend
Fiber suspended
After suspend, got: hello
Done
