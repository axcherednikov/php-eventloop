--TEST--
Suspension cannot be constructed directly
--EXTENSIONS--
eventloop
--FILE--
<?php

use EventLoop\Suspension;

try {
    new Suspension();
} catch (\Error $e) {
    echo "Caught: " . $e->getMessage() . "\n";
}

echo "Done\n";

?>
--EXPECT--
Caught: Cannot directly construct EventLoop\Suspension
Done
