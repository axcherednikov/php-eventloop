--TEST--
EventLoop::delay() and repeat() reject non-finite timer values
--EXTENSIONS--
eventloop
--FILE--
<?php

use EventLoop\EventLoop;

foreach ([NAN, INF] as $value) {
    try {
        EventLoop::delay($value, function () {});
    } catch (\ValueError $e) {
        echo "delay rejected\n";
    }
}

foreach ([NAN, INF] as $value) {
    try {
        EventLoop::repeat($value, function () {});
    } catch (\ValueError $e) {
        echo "repeat rejected\n";
    }
}

try {
    EventLoop::delay(PHP_FLOAT_MAX, function () {});
} catch (\ValueError $e) {
    echo "delay too large\n";
}

try {
    EventLoop::repeat(PHP_FLOAT_MAX, function () {});
} catch (\ValueError $e) {
    echo "repeat too large\n";
}

echo "Done\n";

?>
--EXPECT--
delay rejected
delay rejected
repeat rejected
repeat rejected
delay too large
repeat too large
Done
