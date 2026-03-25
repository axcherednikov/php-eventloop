--TEST--
EventLoop::getIdentifiers() returns all active callback IDs
--EXTENSIONS--
eventloop
--FILE--
<?php

use EventLoop\EventLoop;

$id1 = EventLoop::defer(function () {});
$id2 = EventLoop::delay(1.0, function () {});
$id3 = EventLoop::repeat(1.0, function () {});

$ids = EventLoop::getIdentifiers();
echo "Count: " . count($ids) . "\n";
echo "Contains defer: " . (in_array($id1, $ids) ? "yes" : "no") . "\n";
echo "Contains delay: " . (in_array($id2, $ids) ? "yes" : "no") . "\n";
echo "Contains repeat: " . (in_array($id3, $ids) ? "yes" : "no") . "\n";

EventLoop::cancel($id2);

$ids = EventLoop::getIdentifiers();
echo "After cancel count: " . count($ids) . "\n";
echo "Still contains delay: " . (in_array($id2, $ids) ? "yes" : "no") . "\n";

EventLoop::cancel($id1);
EventLoop::cancel($id3);

?>
--EXPECT--
Count: 3
Contains defer: yes
Contains delay: yes
Contains repeat: yes
After cancel count: 2
Still contains delay: no
