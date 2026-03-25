--TEST--
EventLoop::getType() returns correct CallbackType
--EXTENSIONS--
eventloop
--FILE--
<?php

use EventLoop\EventLoop;
use EventLoop\CallbackType;

$deferId = EventLoop::defer(function () {});
echo "defer: " . (EventLoop::getType($deferId) === CallbackType::Defer ? "ok" : "fail") . "\n";

$delayId = EventLoop::delay(1.0, function () {});
echo "delay: " . (EventLoop::getType($delayId) === CallbackType::Delay ? "ok" : "fail") . "\n";

$repeatId = EventLoop::repeat(1.0, function () {});
echo "repeat: " . (EventLoop::getType($repeatId) === CallbackType::Repeat ? "ok" : "fail") . "\n";

// Cancel all to let tests finish
EventLoop::cancel($deferId);
EventLoop::cancel($delayId);
EventLoop::cancel($repeatId);

echo "Done\n";

?>
--EXPECT--
defer: ok
delay: ok
repeat: ok
Done
