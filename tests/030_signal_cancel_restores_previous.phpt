--TEST--
EventLoop::cancel() restores the previous signal handler
--EXTENSIONS--
eventloop
pcntl
posix
--FILE--
<?php

use EventLoop\EventLoop;

pcntl_async_signals(false);
pcntl_signal(SIGUSR1, function () {
    echo "previous\n";
});

$id = EventLoop::onSignal(SIGUSR1, function () {
    echo "eventloop\n";
});

EventLoop::cancel($id);

posix_kill(posix_getpid(), SIGUSR1);
pcntl_signal_dispatch();

echo "done\n";

?>
--EXPECT--
previous
done
