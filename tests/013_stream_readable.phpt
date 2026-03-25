--TEST--
EventLoop::onReadable() monitors stream
--EXTENSIONS--
eventloop
--FILE--
<?php

use EventLoop\EventLoop;

$pair = stream_socket_pair(STREAM_PF_UNIX, STREAM_SOCK_STREAM, STREAM_IPPROTO_IP);
stream_set_blocking($pair[0], false);
stream_set_blocking($pair[1], false);

$readId = EventLoop::onReadable($pair[0], function (string $callbackId) use ($pair) {
    $data = fread($pair[0], 1024);
    echo "Read: $data\n";
    EventLoop::cancel($callbackId);
});

// Write data to trigger readable event
EventLoop::defer(function () use ($pair) {
    fwrite($pair[1], "hello from eventloop");
});

EventLoop::run();

fclose($pair[0]);
fclose($pair[1]);

echo "Done\n";

?>
--EXPECT--
Read: hello from eventloop
Done
