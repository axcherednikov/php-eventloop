--TEST--
EventLoop::getDriver() returns driver name
--EXTENSIONS--
eventloop
--FILE--
<?php

use EventLoop\EventLoop;

$driver = EventLoop::getDriver();
echo "Driver is string: " . (is_string($driver) ? "yes" : "no") . "\n";
echo "Driver not empty: " . (strlen($driver) > 0 ? "yes" : "no") . "\n";
echo "Driver is known: " . (in_array($driver, ['epoll', 'kqueue', 'poll', 'select']) ? "yes" : "no") . "\n";

?>
--EXPECT--
Driver is string: yes
Driver not empty: yes
Driver is known: yes
