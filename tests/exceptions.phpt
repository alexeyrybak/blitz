--TEST--
errors as exceptions
--FILE--
<?php

include('common.inc');
ini_set('blitz.throw_exceptions', 1);

try {
    $T = new Blitz('nofile.tpl');
} catch (Exception $e) {
    echo 'Exception: '. $e->getMessage()."\n";
}

?>
--EXPECTF--	
blitz::blitz(): unable to open file "%snofile.tpl"
Exception: unable to open file "%snofile.tpl"
