--TEST--
Exception in callback stops loop when no error handler set
--EXTENSIONS--
eventloop
--FILE--
<?php

use EventLoop\EventLoop;

EventLoop::defer(function () {
    throw new RuntimeException("unhandled error");
});

EventLoop::defer(function () {
    echo "This should not execute\n";
});

try {
    EventLoop::run();
} catch (RuntimeException $e) {
    echo "Caught after run: " . $e->getMessage() . "\n";
}

echo "Done\n";

?>
--EXPECT--
Caught after run: unhandled error
Done
