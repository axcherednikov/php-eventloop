--TEST--
EventLoop::onWritable() monitors stream
--EXTENSIONS--
eventloop
--FILE--
<?php

use EventLoop\EventLoop;

$pair = stream_socket_pair(STREAM_PF_UNIX, STREAM_SOCK_STREAM, STREAM_IPPROTO_IP);
stream_set_blocking($pair[0], false);
stream_set_blocking($pair[1], false);

$writeId = EventLoop::onWritable($pair[1], function (string $callbackId) use ($pair) {
    fwrite($pair[1], "written");
    echo "Write callback fired\n";
    EventLoop::cancel($callbackId);
});

EventLoop::run();

// Read what was written
$data = fread($pair[0], 1024);
echo "Read back: $data\n";

fclose($pair[0]);
fclose($pair[1]);

?>
--EXPECT--
Write callback fired
Read back: written
