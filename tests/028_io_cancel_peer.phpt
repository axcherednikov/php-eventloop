--TEST--
I/O watchers can cancel each other while events are being dispatched
--EXTENSIONS--
eventloop
--FILE--
<?php

use EventLoop\EventLoop;

$pair = stream_socket_pair(STREAM_PF_UNIX, STREAM_SOCK_STREAM, STREAM_IPPROTO_IP);
stream_set_blocking($pair[0], false);
stream_set_blocking($pair[1], false);

$readId = null;
$writeId = null;

$readId = EventLoop::onReadable($pair[0], function () use ($pair, &$readId, &$writeId) {
    fread($pair[0], 1024);
    EventLoop::cancel($readId);
    EventLoop::cancel($writeId);
    EventLoop::stop();
});

$writeId = EventLoop::onWritable($pair[1], function () use (&$readId, &$writeId) {
    EventLoop::cancel($readId);
    EventLoop::cancel($writeId);
    EventLoop::stop();
});

fwrite($pair[1], "ready");

EventLoop::run();

fclose($pair[0]);
fclose($pair[1]);

echo "Done\n";

?>
--EXPECT--
Done
