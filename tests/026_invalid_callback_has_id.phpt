--TEST--
InvalidCallbackError exposes callbackId property
--EXTENSIONS--
eventloop
--FILE--
<?php

use EventLoop\EventLoop;
use EventLoop\InvalidCallbackError;

try {
    EventLoop::enable("test_id_123");
} catch (InvalidCallbackError $e) {
    echo "callbackId: " . $e->callbackId . "\n";
    echo "message: " . $e->getMessage() . "\n";
    echo "is Error: " . ($e instanceof \Error ? "yes" : "no") . "\n";
}

?>
--EXPECT--
callbackId: test_id_123
message: Invalid callback identifier "test_id_123"
is Error: yes
