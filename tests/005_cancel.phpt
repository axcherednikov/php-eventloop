--TEST--
EventLoop::cancel() prevents callback execution
--EXTENSIONS--
eventloop
--FILE--
<?php

use EventLoop\EventLoop;

$id = EventLoop::defer(function () {
    echo "This should not execute\n";
});

EventLoop::cancel($id);

EventLoop::defer(function () {
    echo "This should execute\n";
});

EventLoop::run();

echo "Done\n";

?>
--EXPECT--
This should execute
Done
