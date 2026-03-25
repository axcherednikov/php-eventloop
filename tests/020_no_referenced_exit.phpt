--TEST--
EventLoop::run() exits when no referenced callbacks remain
--EXTENSIONS--
eventloop
--FILE--
<?php

use EventLoop\EventLoop;

// No callbacks at all - should exit immediately
EventLoop::run();
echo "Exited with no callbacks\n";

// Only unreferenced callbacks - should exit immediately
$id = EventLoop::repeat(0.01, function () {
    echo "This should not execute\n";
});
EventLoop::unreference($id);

EventLoop::run();
echo "Exited with unreferenced only\n";

EventLoop::cancel($id);

?>
--EXPECT--
Exited with no callbacks
Exited with unreferenced only
