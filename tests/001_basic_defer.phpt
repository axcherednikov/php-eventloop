--TEST--
EventLoop::defer() executes callback
--EXTENSIONS--
eventloop
--FILE--
<?php

use EventLoop\EventLoop;

$executed = false;

EventLoop::defer(function (string $id) use (&$executed) {
    echo "Deferred callback executed\n";
    echo "Got callback ID: " . (strlen($id) > 0 ? "yes" : "no") . "\n";
    $executed = true;
});

EventLoop::run();

var_dump($executed);

?>
--EXPECT--
Deferred callback executed
Got callback ID: yes
bool(true)
