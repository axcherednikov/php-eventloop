--TEST--
Deferred callbacks can schedule new deferred callbacks
--EXTENSIONS--
eventloop
--FILE--
<?php

use EventLoop\EventLoop;

EventLoop::defer(function () {
    echo "First defer\n";
    EventLoop::defer(function () {
        echo "Nested defer\n";
    });
});

EventLoop::run();

echo "Done\n";

?>
--EXPECT--
First defer
Nested defer
Done
