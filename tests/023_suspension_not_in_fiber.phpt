--TEST--
Suspension::suspend() throws when not in a fiber
--EXTENSIONS--
eventloop
--FILE--
<?php

use EventLoop\EventLoop;

$suspension = EventLoop::getSuspension();

try {
    $suspension->suspend();
} catch (\Error $e) {
    echo "Caught: " . $e->getMessage() . "\n";
}

echo "Done\n";

?>
--EXPECT--
Caught: EventLoop\Suspension::suspend() must be called from within a fiber
Done
