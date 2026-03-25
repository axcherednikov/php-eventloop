--TEST--
EventLoop::unreference() allows loop to exit
--EXTENSIONS--
eventloop
--FILE--
<?php

use EventLoop\EventLoop;

// This repeat would keep the loop alive forever, but unreference lets it exit
$id = EventLoop::repeat(0.01, function () {
    echo "This should not execute\n";
});

echo "isReferenced: " . var_export(EventLoop::isReferenced($id), true) . "\n";

EventLoop::unreference($id);

echo "isReferenced after unreference: " . var_export(EventLoop::isReferenced($id), true) . "\n";

// Loop should exit immediately since there are no referenced callbacks
EventLoop::run();

echo "Loop exited\n";

?>
--EXPECT--
isReferenced: true
isReferenced after unreference: false
Loop exited
