--TEST--
EventLoop throws InvalidCallbackError for unknown callback ID
--EXTENSIONS--
eventloop
--FILE--
<?php

use EventLoop\EventLoop;
use EventLoop\InvalidCallbackError;

try {
    EventLoop::enable("nonexistent");
} catch (InvalidCallbackError $e) {
    echo "Caught: " . $e->getMessage() . "\n";
}

try {
    EventLoop::disable("nonexistent");
} catch (InvalidCallbackError $e) {
    echo "Caught: " . $e->getMessage() . "\n";
}

try {
    EventLoop::getType("nonexistent");
} catch (InvalidCallbackError $e) {
    echo "Caught: " . $e->getMessage() . "\n";
}

// cancel() should not throw
EventLoop::cancel("nonexistent");
echo "cancel() did not throw\n";

?>
--EXPECT--
Caught: Invalid callback identifier "nonexistent"
Caught: Invalid callback identifier "nonexistent"
Caught: Invalid callback identifier "nonexistent"
cancel() did not throw
